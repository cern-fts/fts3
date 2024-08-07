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

#include "OptimizerExecutor.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"

using namespace fts3::common;


namespace fts3 {
namespace optimizer {


// borrowed from http://oroboro.com/irregular-ema/
static double exponentialMovingAverage(double sample, double alpha, double cur)
{
    if (sample > 0) {
        cur = (sample * alpha) + ((1 - alpha) * cur);
    }

    return cur;
}


static boost::posix_time::time_duration calculateTimeFrame(time_t avgDuration)
{
    if (avgDuration > 0 && avgDuration < 30) {
        return boost::posix_time::minutes(5);
    }

    if (avgDuration > 30 && avgDuration < 900) {
        return boost::posix_time::minutes(15);
    }

    return boost::posix_time::minutes(30);
}


void OptimizerExecutor::getOptimizerWorkingRange(Range& range, StorageLimits& limits)
{
    // Query specific limits
    dataSource->getPairLimits(pair, range, limits);

    // If range not set, use defaults
    if (range.min <= 0) {
        if (pair.isLanTransfer()) {
            range.min = DEFAULT_LAN_ACTIVE;
        } else {
            range.min = DEFAULT_MIN_ACTIVE;
        }
    }

    bool isMaxConfigured = (range.max > 0);

    if (!isMaxConfigured) {
        range.max = std::min({limits.source, limits.destination});
        range.storageSpecific = true;
        if (range.max < range.min) {
            range.max = range.min;
        }
    }

    BOOST_ASSERT(range.min > 0 && range.max >= range.min);
}

// To be called for low success rates (<= LOW_SUCCESS_RATE)
static int optimizeLowSuccessRate(const PairState &current, const PairState &previous, int previousValue,
    int baseSuccessRate, int decreaseStepSize, std::stringstream& rationale)
{
    int decision = previousValue;
    // If improving, keep it stable
    if (current.successRate > previous.successRate && current.successRate >= baseSuccessRate &&
        current.retryCount <= previous.retryCount) {
        rationale << "Bad link efficiency but progressively improving";
    }
        // If worse or the same, step back
    else if (current.successRate < previous.successRate) {
        decision = previousValue - decreaseStepSize;
        rationale << "Bad link efficiency";
    } else {
        decision = previousValue - decreaseStepSize;
        rationale << "Bad link efficiency, no changes";
    }

    return decision;
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
    int decreaseStepSize, int increaseStepSize, std::stringstream& rationale)
{
    int decision = previousValue;

    if (current.queueSize < previousValue) {
        rationale << "Queue emptying. Hold on.";
    } else if (current.ema < previous.ema) {
        // If the throughput is worsening, we need to look at the file sizes.
        // If the file sizes are decreasing, then it could be that the throughput deterioration is due to
        // this. Thus, decreasing the number of actives will be a bad idea.
        if (round(log10(current.filesizeAvg)) < round(log10(previous.filesizeAvg))) {
            decision = previousValue + increaseStepSize;
            rationale << "Good link efficiency, throughput deterioration, avg. filesize decreasing";
        }
        // Compare on the logarithmic scale, to reduce sensitivity
        else if (round(log10(current.ema)) < round(log10(previous.ema))) {
            decision = previousValue - decreaseStepSize;
            rationale << "Good link efficiency, throughput deterioration";
        }
        // We have lost an order of magnitude, so drop actives
        else {
            decision = previousValue;
            rationale << "Good link efficiency, small throughput deterioration";
        }
    } else if (current.ema > previous.ema) {
        decision = previousValue + increaseStepSize;
        rationale << "Good link efficiency, current average throughput is larger than the preceding average";
    } else {
        decision = previousValue + increaseStepSize;
        rationale << "Good link efficiency. Increment";
    }

    return decision;
}

// This algorithm idea is similar to the TCP congestion window.
// It gives priority to success rate. If it gets worse, it will back off reducing
// the total number of connections between storages.
// If the success rate is good, and the throughput improves, it will increase the number
// of connections.
bool OptimizerExecutor::optimizeConnectionsForPair(OptimizerMode optMode)
{
    std::stringstream rationale;
    bool firstRun = false;
    int decision;

    // Start ticking!
    auto startTime = std::chrono::steady_clock::now();

    // Optimizer working values
    Range range;
    StorageLimits limits;
    getOptimizerWorkingRange(range, limits);

    // Previous decision
    int previousValue = dataSource->getOptimizerValue(pair);

    // Initialize current state
    PairState current;
    current.timestamp = time(NULL);
    current.avgDuration = dataSource->getAverageDuration(pair, boost::posix_time::minutes(30));

    boost::posix_time::time_duration timeFrame = calculateTimeFrame(current.avgDuration);

    dataSource->getThroughputInfo(pair, timeFrame,
        &current.throughput, &current.filesizeAvg, &current.filesizeStdDev);
    current.successRate = dataSource->getSuccessRateForPair(pair, timeFrame, &current.retryCount);
    current.activeCount = dataSource->getActive(pair);
    current.queueSize = dataSource->getSubmitted(pair);

    // There is no value yet. In this case, pick the high value if configured, mid-range otherwise.
    if (previousValue == 0) {
        if (range.specific) {
            decision = range.max;
            rationale << "No information. Use configured range max.";
        } else {
            decision = range.min + (range.max - range.min) / 2;
            rationale << "No information. Start halfway.";
        }

        setOptimizerDecision(current, decision, decision, rationale.str(), startTime);

        current.ema = current.throughput;
        inMemoryStore[pair] = current;

        return true;
    }

    // There is information, but it is the first time seen since the restart
    if (inMemoryStore.find(pair) == inMemoryStore.end()) {
        current.ema = current.throughput;
        inMemoryStore[pair] = current;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Store first feedback from " << pair << commit;
        firstRun = true;
    }

    const PairState previous = inMemoryStore[pair];

    // Calculate new Exponential Moving Average
    current.ema = exponentialMovingAverage(current.throughput, emaAlpha, previous.ema);

    // If we have no range, leave it here
    if (range.min == range.max) {
        setOptimizerDecision(current, range.min, 0, "Range fixed", startTime);
        return true;
    }

    // Apply bandwidth limits
    if (limits.throughputSource > 0) {
        double throughput = dataSource->getThroughputAsSource(pair.source);

        if (throughput > limits.throughputSource) {
            decision = previousValue - decreaseStepSize;
            rationale << "Source throughput limitation reached (" << limits.throughputSource << ")";
            setOptimizerDecision(current, decision, 0, rationale.str(), startTime);
            return true;
        }
    }

    if (limits.throughputDestination > 0) {
        double throughput = dataSource->getThroughputAsDestination(pair.destination);

        if (throughput > limits.throughputDestination) {
            decision = previousValue - decreaseStepSize;
            rationale << "Destination throughput limitation reached (" << limits.throughputDestination << ")";
            setOptimizerDecision(current, decision, 0, rationale.str(), startTime);
            return true;
        }
    }

    // Run only when it makes sense
    time_t timeSinceLastUpdate = current.timestamp - previous.timestamp;

    if (!firstRun &&
        current.successRate == previous.successRate &&
        current.ema == previous.ema &&
        timeSinceLastUpdate < optimizerSteadyInterval.total_seconds()) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
            << "Optimizer for " << pair
            << ": Same success rate and throughput EMA, not enough time passed since last update. Skip"
            << commit;
        return false;
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
        << "Optimizer max possible number of actives for " << pair << " " << range.max << commit;

    // For low success rates, do not even care about throughput
    if (current.successRate < lowSuccessRate) {
        decision = optimizeLowSuccessRate(current, previous, previousValue,
            baseSuccessRate, decreaseStepSize,
            rationale);
    }
    // No throughput info
    else if (current.ema == 0) {
        decision = optimizeNotEnoughInformation(current, previous, previousValue, rationale);
    }
    // Good success rate, or not enough information to take any decision
    else {
        int localIncreaseStep = increaseStepSize;

        if (optMode >= kOptimizerNormal) {
            localIncreaseStep = increaseAggressiveStepSize;
        }

        decision = optimizeGoodSuccessRate(current, previous, previousValue,
            decreaseStepSize, localIncreaseStep,
            rationale);
    }

    // Apply margins to the decision
    if (decision < range.min) {
        decision = range.min;
        rationale << ". Hit lower range limit";

        if (!range.specific) {
            rationale <<". Using *->* link configuration";
        }
    }
    else if (decision > range.max) {
        decision = range.max;
        rationale << ". Hit upper range limit";

        if (!range.specific) {
            rationale <<". Using *->* link configuration";
        } else if (range.storageSpecific) {
            rationale <<". Link not configured. Using SE limits";
        }
    }

    // We may have a higher number of connections than available on the queue.
    // If stream optimize is enabled, push forward and let those extra connections
    // go into streams.
    // Otherwise, stop pushing.
    if (optMode == kOptimizerConservative) {
        if (decision > previousValue && current.queueSize < decision) {
            decision = previousValue;
            rationale << ". Not enough files in the queue";
        }
    }
    // Do not go too far with the number of connections
    if (optMode >= kOptimizerNormal) {
        const bool want_to_increase = decision > previousValue;

        if (want_to_increase && decision > current.queueSize * maxNumberOfStreams) {
            decision = previousValue;
            rationale << ". Too many streams";
        }
    }

    BOOST_ASSERT(decision > 0);
    BOOST_ASSERT(!rationale.str().empty());

    setOptimizerDecision(current, decision, decision - previousValue, rationale.str(), startTime);
    return true;
}


void OptimizerExecutor::setOptimizerDecision(const PairState& current, const int decision, const int diff,
                                             const std::string& rationale,
                                             const std::chrono::steady_clock::time_point& start)
{
    const auto now = std::chrono::steady_clock::now();
    const double duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - start).count();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)
        << "Optimizer decision: Pair " << pair
        << " decision=" << decision << " running=" << current.activeCount << " diff=" << diff
        << " rationale=\"" << rationale << "\"" << " (" << duration * 1000 << "s)" << commit;

    inMemoryStore[pair] = current;
    inMemoryStore[pair].connections = decision;
    dataSource->storeOptimizerDecision(pair, decision, current, diff, rationale);

    if (callbacks) {
        callbacks->notifyDecision(pair, decision, current, diff, rationale);
    }
}

}
}
