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


// borrowed from http://oroboro.com/irregular-ema/
static inline double exponentialMovingAverage(double sample, double alpha, double cur)
{
    if (sample > 0)
        cur = (sample * alpha) + ((1 - alpha) * cur);
    return cur;
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

// To be called for low success rates (<= LOW_SUCCESS_RATE)
static int optimizeLowSuccessRate(const PairState &current, const PairState &previous, int previousValue,
    std::stringstream& rationale)
{
    int decision = previousValue;

    // If improving, keep it stable
    if (current.successRate > previous.successRate && current.successRate >= BASE_SUCCESS_RATE &&
        current.retryCount <= previous.retryCount) {
        rationale << "Bad link efficiency but progressively improving";
    }
        // If worse or the same, step back
    else if (current.successRate < previous.successRate) {
        decision = previousValue - 2;
        rationale << "Bad link efficiency";
    }
    else {
        decision = previousValue - 2;
        rationale << "Bad link efficiency, no changes";
    }

    return decision;
}

// To be called for success rates higher than low, but worsening
static int optimizeWorseningSuccessRate(const PairState &, const PairState &, int previousValue,
    std::stringstream& rationale)
{
    rationale << "Worse link efficiency";
    return previousValue - 1;
}

// To be called when there is not enough information to decide what to do
static int optimizeNotEnoughInformation(const PairState &, const PairState &, int previousValue,
    std::stringstream& rationale)
{
    rationale << "Steady, not enough throughput information";
    return previousValue;
}

// To be called when the success rate is good
static int optimizeGoodSuccessRate(const PairState &current, const PairState &previous, int previousValue,
    int optimizerMode, std::stringstream& rationale)
{
    int decision;

    if (current.ema < previous.ema) {
        // If the throughput is worsening, we need to look at the file sizes.
        // If the file sizes are decreasing, then it could be that the throughput deterioration is due to
        // this. Thus, decreasing the number of actives will be a bad idea.
        if (round(log(current.filesizeAvg)) < round(log(previous.filesizeAvg))) {
            decision = previousValue;
            rationale << "Good link efficiency, throughput deterioration, avg. filesize decreasing";
        } else {
            decision = previousValue - 1;
            rationale << "Good link efficiency, throughput deterioration";
        }
    }
    else if (current.ema > previous.ema) {
        if (optimizerMode >= 2) {
            decision = previousValue + 2;
        }
        else {
            decision = previousValue + 1;
        }
        rationale << "Good link efficiency, current average throughput is larger than the preceding average";
    }
    else {
        decision = previousValue + 1;
        rationale << "Good link efficiency. Increment";
    }

    return decision;
}

// This algorithm idea is similar to the TCP congestion window.
// It gives priority to success rate. If it gets worse, it will back off reducing
// the total number of connections between storages.
// If the success rate is good, and the throughput improves, it will increase the number
// of connections.
void Optimizer::optimizeConnectionsForPair(const Pair &pair)
{
    // Optimizer working values
    Range range;
    Limits limits;
    bool isRangeConfigured = getOptimizerWorkingRange(pair, &range, &limits);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer range for " << pair << ": " << range  << commit;

    // Previous decision
    int previousValue = dataSource->getOptimizerValue(pair);

    // Initialize current state
    PairState current;
    current.timestamp = time(NULL);
    dataSource->getThroughputInfo(pair, boost::posix_time::minutes(1), &current.throughput,
        &current.filesizeAvg,
        &current.filesizeStdDev);
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

    const PairState previous = inMemoryStore[pair];

    // Calculate new Exponential Moving Average
    current.ema = exponentialMovingAverage(current.throughput, EMA_ALPHA, previous.ema);

    // If we have no range, leave it here
    if (range.min == range.max) {
        storeOptimizerDecision(pair, range.min, current, 0, "Range fixed");
        return;
    }

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

        storeOptimizerDecision(pair, decision, current, decision, rationale.str());
        return;
    }

    // Apply bandwidth limits
    if (limits.throughputSource > 0) {
        double throughput = dataSource->getThroughputAsSource(pair.source);
        if (throughput > limits.throughputSource) {
            decision = previousValue - 1;
            rationale << "Source throughput limitation reached (" << limits.throughputSource << ")";
            storeOptimizerDecision(pair, decision, current, 0, rationale.str());
            return;
        }
    }
    if (limits.throughputDestination > 0) {
        double throughput = dataSource->getThroughputAsDestination(pair.destination);
        if (throughput > limits.throughputDestination) {
            decision = previousValue - 1;
            rationale << "Destination throughput limitation reached (" << limits.throughputDestination << ")";
            storeOptimizerDecision(pair, decision, current, 0, rationale.str());
            return;
        }
    }

    // Run only when it makes sense
    time_t timeSinceLastUpdate = current.timestamp - previous.timestamp;

    if (current.successRate == previous.successRate &&
        current.ema == previous.ema &&
        timeSinceLastUpdate < optimizerSteadyInterval) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
            << "Optimizer for " << pair
            << ": Same success rate and throughput EMA, not enough time passed since last update. Skip"
            << commit;
        return;
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
        << "Optimizer max possible number of actives for " << pair << ": " << range.max << commit;

    // For low success rates, do not even care about throughput
    if (current.successRate < LOW_SUCCESS_RATE) {
        decision = optimizeLowSuccessRate(current, previous, previousValue, rationale);
    }
    // Success rate worsening
    else if (current.successRate >= LOW_SUCCESS_RATE && current.successRate < previous.successRate) {
        decision = optimizeWorseningSuccessRate(current, previous, previousValue, rationale);
    }
    // No throughput info
    else if (current.ema == 0) {
        decision = optimizeNotEnoughInformation(current, previous, previousValue, rationale);
    }
    // Good success rate
    else if ((current.successRate == MAX_SUCCESS_RATE ||
             (current.successRate >= MED_SUCCESS_RATE && current.successRate >= previous.successRate)) &&
             current.retryCount <= previous.retryCount)  {
        decision = optimizeGoodSuccessRate(current, previous, previousValue, optimizerMode, rationale);
    }
    // Not enough information to take any decision
    else {
        decision = optimizeGoodSuccessRate(current, previous, previousValue, optimizerMode, rationale);
    }

    // Apply margins to the decision
    if (decision < range.min) {
        decision = range.min;
        rationale << ". Hit lower range limit";
    }
    else if (decision > range.max) {
        decision = range.max;
        rationale << ". Hit upper range limit";
    }

    // We may have a higher number of connections than available on the queue.
    // If stream optimize is enabled, push forward and let those extra connections
    // go into streams.
    // Otherwise, stop pushing.
    if (optimizerMode <= 1) {
        if (decision > previousValue && current.queueSize < decision) {
            decision = previousValue;
            rationale << ". Not enough files in the queue";
        }
    }

    BOOST_ASSERT(decision > 0);
    BOOST_ASSERT(!rationale.str().empty());

    storeOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str());
}


void Optimizer::storeOptimizerDecision(const Pair &pair, int decision, const PairState &current,
    int diff, const std::string &rationale)
{
    inMemoryStore[pair] = current;
    inMemoryStore[pair].connections = decision;
    dataSource->storeOptimizerDecision(pair, decision, current, diff, rationale);
}

}
}