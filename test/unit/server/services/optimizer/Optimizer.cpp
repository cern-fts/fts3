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
#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

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
typedef std::list<Pair> reversedNetlinkTraces;

class BaseOptimizerFixture: public OptimizerDataSource, public Optimizer {
protected:
    std::map<Pair, OptimizerRegister> registry;
    std::map<Pair, int> streamsRegistry;
    std::map<Pair, TransferList> transferStore;
    std::map<std::string, StorageState> mockStorageStore;
    std::map<std::string, reversedNetlinkTraces> reversedNetlinkTraceStore;

    OptimizerMode mockOptimizerMode;

    void populateTransfers(const Pair &pair, const std::string &state, int count,
        bool recoverable = false, double thr = 10, uint64_t filesize = 1024) {

        auto &transfers = transferStore[pair];

        for (int i = 0; i < count; i++) {
            time_t start = 0, end = 0;
            if (state != "SUBMITTED" && state != "ACTIVE") {
                end = time(NULL) - count;
            }
            if (state != "SUBMITTED") {
                start = time(NULL) - count - 60;
            }

            transfers.emplace_back(start, end, state, filesize, thr, recoverable);
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
    BaseOptimizerFixture(): Optimizer(this, NULL) {
        mockOptimizerMode = kOptimizerDisabled;
    }

    std::list<Pair> getActivePairs(void) {
        std::list<Pair> pairs;
        boost::copy(transferStore | boost::adaptors::map_keys, std::back_inserter(pairs));
        return pairs;
    }

    // Test environment implementation of getStorageStates
    // Gets the storage states from the getStorage limit function 
    // Gets Instant throughput from the getThroughputAsSourceInst and getThroughputAsDestinationInst
    // Returns: A map from SE name (string) --> StorageState (both limits and actual throughput values).
    void getStorageStates(std::map<std::string, StorageState> *result) {
        StorageState SEState;
        StorageLimits limits;
        const Pair pair = getActivePairs().front(); //there is only every 1 pair in the unit tests
        std::string se;
        
        //There is only ever one pair in the unit tests, so the only two elements in the currentSEStateMap are
        //the source and destination of the given pair

        //Creating the SE state for the source
        se = pair.source;
        getStorageLimits(pair, &limits);
        SEState.outboundMaxActive = limits.source;
        SEState.outboundMaxThroughput = limits.throughputSource;
        SEState.inboundMaxActive = limits.source;
        SEState.inboundMaxThroughput = limits.source;

        // Queries test environment to get current instantaneous throughput value.
        if(SEState.outboundMaxThroughput > 0) {
            SEState.asSourceThroughputInst = getThroughputAsSourceInst(se);
        }
        if(SEState.inboundMaxThroughput > 0) { 
            SEState.asDestThroughputInst = getThroughputAsDestinationInst(se);                
        }

        //Stores SEState the map
        (*result)[se] = SEState;
        
        //Creating the SE state for the destination
        se = pair.destination;
        SEState.outboundMaxActive = limits.destination;
        SEState.outboundMaxThroughput = limits.throughputDestination;
        SEState.inboundMaxActive = limits.destination;
        SEState.inboundMaxThroughput = limits.throughputDestination;

        // Queries test environment to get current instantaneous throughput value.
        if(SEState.outboundMaxThroughput > 0) {
            SEState.asSourceThroughputInst = getThroughputAsSourceInst(se);
        }
        if(SEState.inboundMaxThroughput > 0) { 
            SEState.asDestThroughputInst = getThroughputAsDestinationInst(se);                
        }

        //Stores SEState the map
        (*result)[se] = SEState;

        return;
    }   



    // Test environment implementation of -tates
    // Gets the storage states from the getStorage limit function 
    // Gets Instant throughput from the getThroughputAsSourceInst and getThroughputAsDestinationInst
    // Returns: A map from SE name (string) --> StorageState (both limits and actual throughput values).
    void getLinkStates(std::map<std::string, LinkState> *result) {
        LinkState linkState;
        std::map<std::string, LinkLimits> limits;
        const Pair pair = getActivePairs().front(); //there is only every 1 pair in the unit tests
        std::string link;
        
        //There is only ever one pair in the unit tests, so the only two elements in the currentSEStateMap are
        //the source and destination of the given pair

        //Creating the Link state for the source
        link = "link1";
        getLinkLimits(pair, &limits);
        linkState.maxActive = limits[link].active;
        linkState.maxThroughput = limits[link].throughput;
    

        // Queries test environment to get current instantaneous throughput value.
        if(linkState.maxThroughput > 0) {
            linkState.throughputInst = getThroughputOverNetlinkInst(link);
        }

        //Stores SEState the map
        (*result)[link] = linkState;

        return;
    }   


    void populateNetlinkTraces(const Pair &pair, const std::string &netlink) {
        auto &pairs = reversedNetlinkTraceStore[netlink];
        pairs.emplace_back(pair);
    }

    OptimizerMode getOptimizerMode(const std::string&, const std::string&) {
        return mockOptimizerMode;
    }

    bool isRetryEnabled(void) {
        return false;
    }

    void getStorageLimits(const Pair&, StorageLimits *limits) {
        limits->destination = limits->source = 200;
        limits->throughputDestination = limits->throughputSource = 0;
    }

    void getLinkLimits(const Pair& pair, std::map<std::string, LinkLimits> *limits) {
        std::string link = "link1"; 
        (*limits)[link].active = 200; // to change 
        (*limits)[link].throughput = 0;
    }

    void getPairLimits(const Pair&, Range *range) {
        range->min = range->max = 0;
    }

    int getOptimizerValue(const Pair &pair) {
        auto i = registry.find(pair);
        if (i == registry.end() || i->second.empty()) {
            return 0;
        }
        return i->second.back().activeDecision;
    }

    void getThroughputInfo(const Pair &pair, const boost::posix_time::time_duration &interval,
        double *throughput, double *filesizeAvg, double *filesizeStdDev)
    {
        *throughput = *filesizeAvg = *filesizeStdDev = 0;

        auto tsi = transferStore.find(pair);
        if (tsi == transferStore.end()) {
            return;
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

        if (totalSize > 0 && transfers.size() > 0) {
            *throughput = acc / totalSize;
            *filesizeAvg = totalSize / transfers.size();
        }

        return;
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

    std::list<std::string> getLinks(const Pair &pair) {
        auto tsi = transferStore.find(pair);
        // todo: create links and select the minimum throughput cap link 

        return {"link1"};
    }

    // In the test environment, throughput=transferred/duration
    // and the two methods are the same.
    double getThroughputAsSourceInst(const std::string &storage) {
        // transferStore is a map from Pair to TransferList
        // TransferList is a list of MockTransfers
        // MockTransfer is a struct with most of the t_file values.
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

    double getThroughputAsDestinationInst(const std::string &storage) {
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

    double getThroughputOverNetlinkInst(const std::string &netlink) {

        double acc = 0;

        auto pairs = reversedNetlinkTraceStore[netlink];

        for (auto i = pairs.begin(); i != pairs.end(); ++i) {
            auto pair = *i;
            auto tsi = transferStore.find(pair);
            if (tsi == transferStore.end()) {
                continue;
            }

            auto &transfers = tsi->second;

            for (auto j = transfers.begin(); j != transfers.end(); ++j) {
                if (j->state == "ACTIVE") {
                    acc += j->throughput;
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
    StorageLimits limits;
    getStorageLimits(pair, &limits);

    getOptimizerWorkingRange(pair, limits, &range);

    BOOST_CHECK(!range.specific);
    BOOST_CHECK_NE(range.max, 0);
    BOOST_CHECK_NE(range.min, 0);
    BOOST_CHECK_EQUAL(range.max, std::min({limits.destination, limits.source}));
    BOOST_CHECK_EQUAL(range.min, DEFAULT_MIN_ACTIVE);
}

// Working range for a pair where both storages are configured
class OptimizerRangeSeFixture: public BaseOptimizerFixture {
public:
    void getStorageLimits(const Pair&, StorageLimits *limits) {
        limits->destination = 60;
        limits->source = 60;
        limits->throughputDestination = limits->throughputSource = 0;
    }

    void getPairLimits(const Pair &pair, Range *range) {
        range->min = range->max = 0;
    }
};

BOOST_FIXTURE_TEST_CASE (optimizerRangeSeConfig, OptimizerRangeSeFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    Range range;
    StorageLimits limits;
    getStorageLimits(pair, &limits);
    getOptimizerWorkingRange(pair, limits, &range);

    BOOST_CHECK(!range.specific);
    BOOST_CHECK_NE(range.max, 0);
    BOOST_CHECK_NE(range.min, 0);
    BOOST_CHECK_EQUAL(range.max, limits.source);
    BOOST_CHECK_EQUAL(range.min, DEFAULT_MIN_ACTIVE);
}

// Working range is configured
class OptimizerRangeSetFixture: public OptimizerRangeSeFixture {
public:
    void getStorageLimits(const Pair&, StorageLimits *limits) {
        limits->destination = 40;
        limits->source = 20;
    }

    void getPairLimits(const Pair&, Range *range) {
        range->min = 150;
        range->max = 200;
        range->specific = true;
    }
};

BOOST_FIXTURE_TEST_CASE (optimizerRangeSetFixture, OptimizerRangeSetFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    Range range;
    StorageLimits limits;
    getStorageLimits(pair, &limits);
    getOptimizerWorkingRange(pair, limits, &range);

    BOOST_CHECK(range.specific);
    BOOST_CHECK_EQUAL(range.max, 200);
    BOOST_CHECK_EQUAL(range.min, 150);
}

// No prior information
BOOST_FIXTURE_TEST_CASE (optimizerFirstRun, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");
    
    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 100);
    populateTransfers(pair, "ACTIVE", 20);

    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    Range range;
    StorageLimits limits;
    getStorageLimits(pair, &limits);
    getOptimizerWorkingRange(pair, limits, &range);

    BOOST_CHECK_LE(lastEntry->activeDecision, range.max);
    BOOST_CHECK_GE(lastEntry->activeDecision, range.min);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets worse, so the number should be reduced
BOOST_FIXTURE_TEST_CASE (optimizerWorseSuccess, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 100);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 100);

    // Run once (first time)
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 20);

    // Feed recoverable failures
    removeTransfers(pair, "ACTIVE", 10);
    populateTransfers(pair, "FAILED", 10, true);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
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

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 100);

    // Run once (first time)
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 20);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_TEST_MESSAGE(lastEntry->rationale);
    BOOST_CHECK_GT(lastEntry->activeDecision, 20);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets better, so the number should be increased, but there aren't
// enough queued. Optimizer mode is 1, so should stay stable.
BOOST_FIXTURE_TEST_CASE (optimizerStreamsMode1, BaseOptimizerFixture)
{
    mockOptimizerMode = kOptimizerConservative;

    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 10);

    // Run once (first time)
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_TEST_MESSAGE(lastEntry->rationale);
    BOOST_CHECK_EQUAL(lastEntry->activeDecision, 40);
    BOOST_CHECK_EQUAL(streamsRegistry[pair], 1);
}

// Success rate gets better, but the queue is emptying
BOOST_FIXTURE_TEST_CASE (optimizerEmptying, BaseOptimizerFixture)
{
    mockOptimizerMode = kOptimizerAggressive;

    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed successful and actives
    populateTransfers(pair, "FINISHED", 96, false, 100);
    populateTransfers(pair, "FAILED", 4, true);
    populateTransfers(pair, "ACTIVE", 20);
    populateTransfers(pair, "SUBMITTED", 30);

    // Run once (first time)
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // New finished
    removeTransfers(pair, "ACTIVE", 15);
    removeTransfers(pair, "FAILED", 2);
    populateTransfers(pair, "FINISHED", 20, false, 150);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_TEST_MESSAGE(lastEntry->rationale);
    BOOST_CHECK_EQUAL(lastEntry->activeDecision, 40);
}

// Throughput gets worse, but the queue is emptying
BOOST_FIXTURE_TEST_CASE (optimizerEmptyingWorseThr, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed a bunch of successful with a big filesize
    populateTransfers(pair, "SUBMITTED", 10);
    populateTransfers(pair, "FINISHED", 2, false, 100, 1024*1024);

    // Run once
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // Another bunch, with smaller filesizes and less throughput
    populateTransfers(pair, "FINISHED", 100, false, 10, 1024*1024);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_TEST_MESSAGE(lastEntry->rationale);
    BOOST_CHECK_EQUAL(lastEntry->activeDecision, 40);
}

// Success rate is good, throughput worsens, but so does avg. filesize.
// The value should increase.
BOOST_FIXTURE_TEST_CASE (optimizerAvgFilesizeDecreases, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);

    // Feed a bunch of successful with a big filesize
    populateTransfers(pair, "FINISHED", 2, false, 100, 1024*1024*1024);

    // Run once
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision
    setOptimizerValue(pair, 40);

    // Another bunch, with smaller filesizes and less throughput
    populateTransfers(pair, "SUBMITTED", 80);
    populateTransfers(pair, "FINISHED", 100, false, 10, 1024);

    // Run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    auto lastEntry = getLastEntry(pair);

    BOOST_TEST_MESSAGE(lastEntry->rationale);
    BOOST_CHECK_GT(lastEntry->activeDecision, 40);
}

// Success rate is good, better throughput, keep streams sane
BOOST_FIXTURE_TEST_CASE (optimizerMaxStreams, BaseOptimizerFixture)
{
    const Pair pair("mock://dpm.cern.ch", "mock://dcache.desy.de");

    //stores the pair as a list so getCurrentIntervalInputState will accept it
    std::list<Pair> pairs;
    pairs.push_back(pair);
    
    maxNumberOfStreams = 4;

    // Feed just a few of successes
    populateTransfers(pair, "FINISHED", 5, false, 100, 1024*1024);
    populateTransfers(pair, "FAILED", 5, false, 0, 1024*1024);

    // Run once
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    // Patch decision, so there would be multiple streams
    setOptimizerValue(pair, 40);

    // Add queued, and more successes with better throughput
    populateTransfers(pair, "FINISHED", 5, false, 200, 1024*1024);
    populateTransfers(pair, "SUBMITTED", 2);

    setMaxNumberOfStreams(4);
    mockOptimizerMode = kOptimizerAggressive;

    //run again
    getCurrentIntervalInputState(pairs); //populates currentPairStateMap, currentSEStateMap
    runOptimizerForPair(pair);

    BOOST_CHECK_LE(streamsRegistry[pair], maxNumberOfStreams);
}

// NOTE: I am not sure it is worth to add more tests. At the end, we will basically be
//       writing tests that set the parameters to fit the implementation at the time.
//       They do not prove that the optimizer optimizes.
//       Still useful to have something that, at the very least, checks it doesn't crash!


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()