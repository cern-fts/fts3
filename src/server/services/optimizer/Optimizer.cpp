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

#include "Optimizer.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace fts3::common;


namespace fts3 {
namespace optimizer {


std::ostream& operator << (std::ostream &os, const Pair &pair) {
    return (os << pair.source << " => " << pair.destination);
}


std::ostream& operator << (std::ostream &os, const Range &range) {
    return (os << range.min << "/" << range.max);
}

// borrowed from http://oroboro.com/irregular-ema/
static inline double exponentialMovingAverage(double sample, double alpha, double cur)
{
    if (sample > 0)
        cur = (sample * alpha) + ((1 - alpha) * cur);
    return cur;
}


Optimizer::Optimizer(OptimizerDataSource *ds):
    dataSource(ds), optimizerSteadyInterval(300),
    globalMaxPerLink(DEFAULT_MAX_ACTIVE_PER_LINK),
    globalMaxPerStorage(DEFAULT_MAX_ACTIVE_ENDPOINT_LINK)
{
}


Optimizer::~Optimizer()
{
}


void Optimizer::setSteadyInterval(int newValue)
{
    optimizerSteadyInterval = newValue;
}


void Optimizer::run(void)
{
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer run" << commit;

    // Query global config once
    globalMaxPerStorage = dataSource->getGlobalStorageLimit();
    if (globalMaxPerStorage <= 0) {
        globalMaxPerStorage = DEFAULT_MAX_ACTIVE_ENDPOINT_LINK;
    }

    globalMaxPerLink = dataSource->getGlobalLinkLimit();
    if (globalMaxPerLink <= 0) {
        globalMaxPerLink = DEFAULT_MAX_ACTIVE_PER_LINK;
    }


    try {
        std::list<Pair> pairs = dataSource->getActivePairs();

        for (auto i = pairs.begin(); i != pairs.end(); ++i) {
            runForPair(*i);
        }
    }
    catch (std::exception &e) {
        throw SystemError(std::string(__func__) + ": Caught exception " + e.what());
    }
    catch (...) {
        throw SystemError(std::string(__func__) + ": Caught exception ");
    }
}


static boost::posix_time::time_duration calculateTimeFrame(time_t avgDuration)
{
    if(avgDuration > 0 && avgDuration < 30) {
        return boost::posix_time::minutes(5);
    }
    else if(avgDuration > 30 && avgDuration < 900) {
        return boost::posix_time::minutes(15);
    }
    else {
        return boost::posix_time::minutes(30);
    }
}


bool Optimizer::getOptimizerWorkingRange(const Pair &pair, Range *range, Limits *limits)
{
    // Query specific limits
    dataSource->getPairLimits(pair, range, limits);

    // If limits not set, use global
    if (limits->source <= 0) {
        limits->source = globalMaxPerStorage;
    }
    if (limits->destination == 0) {
        limits->destination = globalMaxPerStorage;
    }
    if (limits->link == 0) {
        limits->link = globalMaxPerLink;
    }

    // If range not set, use defaults
    if (range->min <= 0) {
        if (pair.isLanTransfer()) {
            range->min = DEFAULT_LAN_ACTIVE;
        }
        else {
            range->min = DEFAULT_MIN_ACTIVE;
        }
    }

    bool isMaxConfigured = (range->max > 0);
    if (!isMaxConfigured) {
        range->max = std::min({limits->source, limits->destination, limits->link});
        if (range->max < range->min) {
            range->max = range->min;
        }
    }

    BOOST_ASSERT(range->min > 0 && range->max >= range->min);
    return isMaxConfigured;
}


void Optimizer::runForPair(const Pair &pair)
{
    // Optimizer working values
    Range range;
    Limits limits;
    bool isRangeConfigured = getOptimizerWorkingRange(pair, &range, &limits);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer range for " << pair << ": " << range  << commit;

    // Parameters we need for the decision
    // TODO: Move outside
    // TODO: Per VO (??)
    int optimizerMode = dataSource->getOptimizerMode();

    // Initialize current state
    PairState current;
    current.timestamp = time(NULL);
    current.throughput = dataSource->getWeightedThroughput(pair, boost::posix_time::minutes(1));
    current.avgDuration = dataSource->getAverageDuration(pair, boost::posix_time::minutes(30));
    current.successRate = dataSource->getSuccessRateForPair(pair,
        calculateTimeFrame(current.avgDuration), &current.retryCount);
    current.activeCount = dataSource->getActive(pair);
    current.queueSize = dataSource->getSubmitted(pair);

    // First time seen, store and exit
    if (inMemoryStore.find(pair) == inMemoryStore.end()) {
        current.ema = current.throughput;
        inMemoryStore[pair] = current;

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Store first feedback from " << pair << commit;
        return;
    }

    const PairState stored = inMemoryStore[pair];

    // Calculate new Exponential Moving Average
    current.ema = ceil(
        exponentialMovingAverage(current.throughput, EMA_ALPHA, stored.ema)
    );

    // Update stored value
    inMemoryStore[pair] = current;

    // If we have no range, leave it here
    if (range.min == range.max) {
        dataSource->storeOptimizerDecision(pair, range.min, 0.0, current, 0, "Range fixed");
        return;
    }

    // Previous decision
    int previousValue = dataSource->getOptimizerValue(pair);

    // New decision
    int decision = 0;
    std::stringstream rationale;

    // There is no value yet. In this case, pick the high value if configured, low otherwise.
    if (previousValue == 0) {
        if (isRangeConfigured) {
            decision = range.max;
            rationale << "No information. Use configured range max.";
        } else {
            decision = range.min;
            rationale << "No information. Use to default min.";
        }

        dataSource->storeOptimizerDecision(pair, decision, 0.0, current, decision, rationale.str());
        return;
    }

    // Apply bandwidth limits
    if (limits.throughputSource > 0) {
        double throughput = dataSource->getThroughputAsSource(pair.source);
        if (throughput > limits.throughputSource) {
            decision = previousValue - 1;
            rationale << "Source throughput limitation reached (" << limits.throughputSource << ")";
            dataSource->storeOptimizerDecision(pair, decision, limits.throughputSource, current, 0, rationale.str());
            return;
        }
    }
    if (limits.throughputDestination > 0) {
        double throughput = dataSource->getThroughputAsDestination(pair.destination);
        if (throughput > limits.throughputDestination) {
            decision = previousValue - 1;
            rationale << "Destination throughput limitation reached (" << limits.throughputDestination << ")";
            dataSource->storeOptimizerDecision(pair, decision, limits.throughputDestination, current, 0, rationale.str());
            return;
        }
    }

    // Run only when it makes sense
    time_t timeSinceLastUpdate = current.timestamp - stored.timestamp;

    if (current.successRate == stored.successRate &&
        current.ema == stored.ema &&
        timeSinceLastUpdate < optimizerSteadyInterval) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
            << "Optimizer for " << pair
            << ": Same success rate and throughput EMA, not enough time passed since last update. Skip"
            << commit;
        return;
    }

    // Count actives for the source and for the destination, to apply storage limits
    int activeForSource = dataSource->getActive(Pair(pair.source, ""));
    int activeForDestination = dataSource->getActive(Pair("", pair.destination));

    // Apply limits
    if (activeForSource >= limits.source) {
        decision = previousValue;
        rationale << "Source limit reached (" << activeForSource << "/" << limits.source << ")";
        dataSource->storeOptimizerDecision(pair, decision, 0.0, current, 0, rationale.str());
        return;
    }
    if (activeForDestination >= limits.destination) {
        decision = previousValue;
        rationale << "Destination limit reached (" << activeForDestination << "/" << limits.destination << ")";
        dataSource->storeOptimizerDecision(pair, decision, 0.0, current, 0, rationale.str());
        return;
    }
    if (current.activeCount >= limits.link) {
        decision = previousValue;
        rationale << "Link limit reached (" << current.activeCount << "/" << limits.link << ")";
        dataSource->storeOptimizerDecision(pair, decision, 0.0, current, 0, rationale.str());
        return;
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
        << "Optimizer max possible number of actives for " << pair << ": " << range.max << commit;

    // For low success rates, do not even care about throughput
    if (current.successRate < LOW_SUCCESS_RATE) {
        // If improving, keep it stable
        if (current.successRate > stored.successRate && current.successRate >= BASE_SUCCESS_RATE &&
            current.retryCount <= stored.retryCount) {
            decision = previousValue;
            rationale << "Bad link efficiency but progressively improving";
        }
        // If worse or the same, step back
        else if (current.successRate < stored.successRate) {
            decision = previousValue - 2;
            rationale << "Bad link efficiency";
        }
        else {
            decision = previousValue - 2;
            rationale << "Bad link efficiency, no changes";
        }
    }
    // Success rate worsening
    else if (current.successRate >= LOW_SUCCESS_RATE && current.successRate < stored.successRate) {
        decision = previousValue - 1;
        rationale << "Worse link efficiency";
    }
    // No throughput info
    else if (current.ema == 0) {
        decision = previousValue;
        rationale << "Steady, not enough throughput information";
    }
    // Good success rate
    else if ((current.successRate == MAX_SUCCESS_RATE ||
             (current.successRate >= MED_SUCCESS_RATE && current.successRate >= stored.successRate)) &&
             current.retryCount <= stored.retryCount)  {
        // Throughput going worse
        if (current.ema < stored.ema) {
            decision = previousValue - 1;
            rationale << "Good link efficiency, throughput deterioration";
        }
        else if (current.ema > stored.ema) {
            if (optimizerMode >= 2) {
                decision = previousValue + 2;
            }
            else {
                decision = previousValue + 1;
            }
            rationale << "Good link efficiency, current average throughput is larger than the preceding average";
        }
        else {
            decision = previousValue;
            rationale << "Good link efficiency, no throughput changes";
        }
    }
    // No changes since previous run, try one more
    else if (current.successRate == stored.successRate && current.ema == stored.ema) {
        decision = previousValue + 1;
        rationale << "Good link efficiency. Increment";
    }
    // Not enough information to take any decision
    else {
        decision = previousValue;
        rationale << "Steady, not enough information";
    }

    // Apply margins to the decision
    if (decision < range.min) {
        decision = range.min;
    }
    else if (decision > range.max) {
        decision = range.max;
        rationale << ". Hit range limit";
    }

    // Do not push further if there are no transfers in the queue
    if (decision > previousValue && current.queueSize < decision) {
        decision = previousValue;
        rationale << ". Not enough files in the queue";
    }

    BOOST_ASSERT(decision > 0);
    BOOST_ASSERT(!rationale.str().empty());

    dataSource->storeOptimizerDecision(pair, decision, 0.0, current, decision - previousValue, rationale.str());
}

}
}
