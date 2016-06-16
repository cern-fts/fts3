/*
 * Copyright (c) CERN 2013-2016
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/test/included/unit_test.hpp>

#include "server/services/optimizer/Optimizer.h"
#include "server/services/optimizer/OptimizerConstants.h"

using namespace fts3::optimizer;

BOOST_AUTO_TEST_SUITE(server)
BOOST_AUTO_TEST_SUITE(OptimizerTestSuite)


struct OptimizerEntry {
    int activeDecision;
    const PairState state;
    int diff;
    std::string rationale;

    OptimizerEntry(int ad, const PairState &ps, int diff, const std::string &r):
        activeDecision(ad), state(ps), diff(diff), rationale(r) {
    }
};
typedef std::list<OptimizerEntry> OptimizerRegister;


struct MockTransfer {
    time_t start, end;
    std::string state;
    uint64_t filesize;
    double throughput;
    bool recoverable;
    int numRetries;

    MockTransfer(time_t start, time_t end, const std::string &state,
        uint64_t filesize, double throughput, bool recoverable):
        start(start), end(end), state(state), filesize(filesize), throughput(throughput), recoverable(recoverable),
        numRetries(0) {
    }
};
typedef std::list<MockTransfer> TransferList;

class BaseOptimizerFixture: public OptimizerDataSource, public Optimizer {
protected:
    std::map<Pair, OptimizerRegister> registry;
    std::map<Pair, int> streamsRegistry;
    std::map<Pair, TransferList> transferStore;

    void populateTransfers(const Pair &pair, const std::string &state, int count,
        bool recoverable = false, double thr = 10) {
        auto &transfers = transferStore[pair];

        for (int i = 0; i < count; i++) {
            time_t start = 0, end = 0;
            if (state != "SUBMITTED" && state != "ACTIVE") {
                end = time(NULL) - count;
            }
            if (state != "SUBMITTED") {
                start = time(NULL) - count - 60;
            }

            transfers.emplace_back(start, end, state, 1024, thr, recoverable);
        }
    }

    void removeTransfers(const Pair &pair, const std::string &state, int count) {
        auto &transfers = transferStore[pair];
        for (auto i = transfers.begin(); i != transfers.end();) {
            if (i->state == state && count > 0) {
                i = transfers.erase(i);
                --count;
            }
            else {
                ++i;
            }
        }
    }

    OptimizerEntry* getLastEntry(const Pair &pair) {
        if (registry[pair].size()) {
            return &registry[pair].back();
        }
        return NULL;
    }

    void setOptimizerValue(const Pair &pair, int value) {
        registry[pair].emplace_back(value, PairState(), value, "Patched");
    }

public:
    BaseOptimizerFixture(): Optimizer(this) {
        globalMaxPerStorage = DEFAULT_MAX_ACTIVE_ENDPOINT_LINK;
        globalMaxPerLink = DEFAULT_MAX_ACTIVE_PER_LINK;
        optimizerMode = 1;
    }

    std::list<Pair> getActivePairs(void) {
        std::list<Pair> pairs;
        boost::copy(transferStore | boost::adaptors::map_keys, std::back_inserter(pairs));
        return pairs;
    }

    int getOptimizerMode(void) {
        return 2;
    }

    bool isRetryEnabled(void) {
        return false;
    }

    int getGlobalStorageLimit(void) {
        return 0;
    }

    int getGlobalLinkLimit(void) {
        return 0;
    }

    void getPairLimits(const Pair&, Range *range, Limits *limits) {
        range->min = range->max = 0;
        memset(limits, 0, sizeof(*limits));
    }

    int getOptimizerValue(const Pair &pair) {
        auto i = registry.find(pair);
        if (i == registry.end() || i->second.empty()) {
            return 0;
        }
        return i->second.back().activeDecision;
    }

    double getThroughput(const Pair &pair, const boost::posix_time::time_duration &interval) {
        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return 0;
        }

        auto &transfers = tsi->second;

        double acc = 0;
        double totalSize = 0;
        time_t notBefore = time(NULL) - interval.total_seconds();

        for (auto i = transfers.begin(); i != transfers.end(); ++i) {
            if (i->state == "ACTIVE" || (i->state == "FINISHED" && i->end >= notBefore)) {
                totalSize += i->filesize;
                acc += i->throughput * i->filesize;
            }
        }

        if (totalSize > 0) {
            acc /= totalSize;
        }

        return acc;
    }

    time_t getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval) {
        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return 0;
        }

        auto &transfers = tsi->second;

        time_t acc = 0;
        int counter = 0;
        time_t notBefore = time(NULL) - interval.total_seconds();

        for (auto i = transfers.begin(); i != transfers.end(); ++i) {
            if (i->end >= notBefore) {
                ++counter;
                acc += (i->end - i->start);
            }
        }

        if (counter == 0) {
            return 0;
        }
        return acc / counter;
    }

    double getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval, int *retryCount) {
        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return 0;
        }

        auto &transfers = tsi->second;

        int nFailedLastHour = 0;
        int nFinishedLastHour = 0;
        time_t notBefore = time(NULL) - interval.total_seconds();

        *retryCount = 0;
        for (auto i = transfers.begin(); i != transfers.end(); ++i) {
            if (i->end >= notBefore) {
                if (i->state == "FAILED" && i->recoverable) {
                    ++nFailedLastHour;
                }
                else if (i->state == "FINISHED") {
                    ++nFinishedLastHour;
                }
                *retryCount += i->numRetries;
            }
        }

        if (nFinishedLastHour > 0) {
            return ceil((nFinishedLastHour * 100.0) / (nFinishedLastHour + nFailedLastHour));
        }
        else {
            return 0.0;
        }
    }

    int getActive(const Pair &pair) {
        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return 0;
        }

        auto &transfers = tsi->second;

        int counter = 0;
        for (auto i = transfers.begin(); i != transfers.end(); ++i) {
            if (i->state == "ACTIVE") {
                ++counter;
            }
        }
        return counter;
    }

    int getSubmitted(const Pair &pair) {
        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return 0;
        }

        auto &transfers = tsi->second;

        int counter = 0;
        for (auto i = transfers.begin(); i != transfers.end(); ++i) {
            if (i->state == "SUBMITTED") {
                ++counter;
            }
        }
        return counter;
    }

    double getThroughputAsSource(const std::string &storage) {
        double acc = 0;

        for (auto i = transferStore.begin(); i != transferStore.end(); ++i) {
            if (i->first.source == storage) {
                auto &transfers = i->second;

                for (auto j = transfers.begin(); j != transfers.end(); ++j) {
                    if (j->state == "ACTIVE") {
                        acc += j->throughput;
                    }
                }
            }
        }
        return acc;
    }

    double getThroughputAsDestination(const std::string &storage) {
        double acc = 0;

        for (auto i = transferStore.begin(); i != transferStore.end(); ++i) {
            if (i->first.destination == storage) {
                auto &transfers = i->second;

                for (auto j = transfers.begin(); j != transfers.end(); ++j) {
                    if (j->state == "ACTIVE") {
                        acc += j->throughput;
                    }
                }
            }
        }
        return acc;
    }

    void storeOptimizerDecision(const Pair &pair, int activeDecision,
        const PairState &newState, int diff, const std::string &rationale) {
        registry[pair].push_back(OptimizerEntry(activeDecision, newState, diff, rationale));
    }

    void storeOptimizerStreams(const Pair &pair, int streams) {
        streamsRegistry[pair] = streams;
    }
};


// Working range for a non configured storage
BOOST_FIXTURE_TEST_CASE (optimizerRangeAllDefaults, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    Range range;
    Limits limits;
    bool maxConfigured = getOptimizerWorkingRange(pair, &range, &limits);

    BOOST_CHECK(!maxConfigured);
    BOOST_CHECK_NE(range.max, 0);
    BOOST_CHECK_NE(range.min, 0);
    BOOST_CHECK_EQUAL(range.max, std::min({limits.link, limits.destination, limits.source}));
    BOOST_CHECK_EQUAL(range.min, DEFAULT_MIN_ACTIVE);
}

// Working range for a pair where both storages are configured
class OptimizerRangeSeFixture: public BaseOptimizerFixture {
public:
    void getPairLimits(const Pair&, Range *range, Limits *limits) {
        range->min = range->max = 0;
        limits->destination = 40;
        limits->source = 20;
        limits->link = 60;
    }
};

BOOST_FIXTURE_TEST_CASE (optimizerRangeSeConfig, OptimizerRangeSeFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    Range range;
    Limits limits;
    bool maxConfigured = getOptimizerWorkingRange(pair, &range, &limits);

    BOOST_CHECK(!maxConfigured);
    BOOST_CHECK_NE(range.max, 0);
    BOOST_CHECK_NE(range.min, 0);
    BOOST_CHECK_EQUAL(range.max, limits.source);
    BOOST_CHECK_EQUAL(range.min, DEFAULT_MIN_ACTIVE);
}

// Working range is configured
class OptimizerRangeSetFixture: public OptimizerRangeSeFixture {
public:
    void getPairLimits(const Pair&, Range *range, Limits *limits) {
        range->min = 150;
        range->max = 200;
        limits->destination = 40;
        limits->source = 20;
    }
};

BOOST_FIXTURE_TEST_CASE (optimizerRangeSetFixture, OptimizerRangeSetFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    Range range;
    Limits limits;
    bool maxConfigured = getOptimizerWorkingRange(pair, &range, &limits);

    BOOST_CHECK(maxConfigured);
    BOOST_CHECK_EQUAL(range.max, 200);
    BOOST_CHECK_EQUAL(range.min, 150);
}

// Success rate gets worse, so the number should be reduced
BOOST_FIXTURE_TEST_CASE (optimizerWorseSuccess, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 100);
    populateTransfers(pair, "ACTIVE", 20);

    // Run once (first time)
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 20);

    // Feed recoverable failures
    removeTransfers(pair, "ACTIVE", 10);
    populateTransfers(pair, "FAILED", 10, true);

    // Run again
    runOptimizerForPair(pair);

    // Should back off
    auto lastEntry = getLastEntry(pair);

    BOOST_CHECK_LT(lastEntry->activeDecision, 20);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets better, so the number should be increased
BOOST_FIXTURE_TEST_CASE (optimizerBetterSuccess, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 100);

    // Run once (first time)
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 20);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_CHECK_GT(lastEntry->activeDecision, 20);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets better, so the number should be increased, but there aren't
// enough queued. Optimizer mode is 1, so should stay stable.
BOOST_FIXTURE_TEST_CASE (optimizerStreamsMode1, BaseOptimizerFixture)
{
    optimizerMode = 1;

    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 10);

    // Run once (first time)
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_CHECK_EQUAL(lastEntry->activeDecision, 40);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets better, so the number should be increased, but there aren't
// enough queued. Optimizer mode is 2, so streams should be increased.
BOOST_FIXTURE_TEST_CASE (optimizerStreamsMode2, BaseOptimizerFixture)
{
    optimizerMode = 2;

    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 10);

    // Run once (first time)
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_CHECK_GT(lastEntry->activeDecision, 40);
    BOOST_CHECK_GT(streamsRegistry[pair], 1);
}

// NOTE: I am not sure it is worth to add more tests. At the end, we will basically be
//       writing tests that set the parameters to fit the implementation at the time.
//       They do not prove that the optimizer optimizes.
//       Still useful to have something that, at the very least, checks it doesn't crash!


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
