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

#include <monitoring/msg-ifce.h>
#include "Optimizer.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include <config/ServerConfig.h>

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


void Optimizer::getOptimizerWorkingRange(const Pair &pair, Range *range, StorageLimits *limits)
{
    // Query specific limits
    dataSource->getPairLimits(pair, range, limits);

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
        range->max = std::min({limits->source, limits->destination});
        range->storageSpecific = true;
        if (range->max < range->min) {
            range->max = range->min;
        }
    }

    BOOST_ASSERT(range->min > 0 && range->max >= range->min);
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
    }
    else {
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
    }
    else if (current.ema < previous.ema) {
        // If the throughput is worsening, we need to look at the file sizes.
        // If the file sizes are decreasing, then it could be that the throughput deterioration is due to
        // this. Thus, decreasing the number of actives will be a bad idea.
        if (round(log10(current.filesizeAvg)) < round(log10(previous.filesizeAvg))) {
            decision = previousValue + increaseStepSize;
            rationale << "Good link efficiency, throughput deterioration, avg. filesize decreasing";
        }
        // Compare on the logarithmic scale, to reduce sensitivity
        else if(round(log10(current.ema)) < round(log10(previous.ema))) {
            decision = previousValue - decreaseStepSize;
            rationale << "Good link efficiency, throughput deterioration";
        }
        // We have lost an order of magnitude, so drop actives
        else {
            decision = previousValue;
            rationale << "Good link efficiency, small throughput deterioration";
        }
    }
    else if (current.ema > previous.ema) {
        decision = previousValue + increaseStepSize;
        rationale << "Good link efficiency, current average throughput is larger than the preceding average";
    }
    else {
        decision = previousValue + increaseStepSize;
        rationale << "Good link efficiency. Increment";
    }

    return decision;
}

void Optimizer::getPairState(const Pair &pair) {
    // Initialize current state
    PairState current;
    current.timestamp = time(NULL);
    current.avgDuration = dataSource->getAverageDuration(pair, boost::posix_time::minutes(30));

    boost::posix_time::time_duration timeFrame = calculateTimeFrame(current.avgDuration);

    current.activeCount = dataSource->getActive(pair);
    current.successRate = dataSource->getSuccessRateForPair(pair, timeFrame, &current.retryCount);
    current.queueSize = dataSource->getSubmitted(pair);
    
    dataSource->getThroughputInfo(pair, timeFrame,
        &current.throughput, &current.filesizeAvg, &current.filesizeStdDev);
    
    current.avgTput = dataSource->getInstThroughputPerConn(pair);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "On " << pair << ", avgThroughput per connection is: " << current.avgTput << commit;
    
    // Saves all the computed information in the local store and database
    stateCollectionStore[pair] = current;
    dataSource->updateOptimizerState(pair, current);
}

void Optimizer::getStorageState(const std::string &se) {
    typedef boost::posix_time::time_duration TDuration;
    auto optimizerInterval = config::ServerConfig::instance().get<TDuration>("OptimizerInterval");

    storageStates[se].asSourceThroughput = dataSource->getThroughputAsSource(se, optimizerInterval);
    storageStates[se].asSourceThroughputInst = dataSource->getThroughputAsSourceInst(se);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Total outbound (window, instantaneous) from " << se << ": (" 
            << storageStates[se].asSourceThroughput << ", " << storageStates[se].asSourceThroughputInst << ")" << commit;

    storageStates[se].asDestThroughput = dataSource->getThroughputAsDestination(se, optimizerInterval);
    storageStates[se].asDestThroughputInst = dataSource->getThroughputAsDestinationInst(se);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Total inbound (window, instantaneous) into " << se << ": (" 
            << storageStates[se].asDestThroughput << ", " << storageStates[se].asDestThroughputInst << ")" << commit;       
    return;
}

// This algorithm idea is similar to the TCP congestion window.
// It gives priority to success rate. If it gets worse, it will back off reducing
// the total number of connections between storages.
// If the success rate is good, and the throughput improves, it will increase the number
// of connections.
bool Optimizer::optimizeConnectionsForPair(OptimizerMode optMode, const Pair &pair)
{
    int decision = 0;
    std::stringstream rationale;

    // Start ticking!
    boost::timer::cpu_timer timer;

    // Read in current pairstate
    PairState current = stateCollectionStore[pair];    

    // Optimizer working values
    Range range;
    StorageLimits limits;
    getOptimizerWorkingRange(pair, &range, &limits);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer range for " << pair << ": " << range  << commit;

    // Previous decision
    int previousValue = dataSource->getOptimizerValue(pair);


    // There is no value yet. In this case, pick the high value if configured, mid-range otherwise.
    if (previousValue == 0) {
        if (range.specific) {
            decision = range.max;
            rationale << "No information. Use configured range max.";
        } else {
            decision = range.min + (range.max - range.min) / 2;
            rationale << "No information. Start halfway.";
        }

        setOptimizerDecision(pair, decision, current, decision, rationale.str(), timer.elapsed());

        current.ema = current.throughput;
        inMemoryStore[pair] = current;

        return true;
    }

    // There is information, but it is the first time seen since the restart
    if (inMemoryStore.find(pair) == inMemoryStore.end()) {
        current.ema = current.throughput;
        inMemoryStore[pair] = current;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Store first feedback from " << pair << commit;
        return false;
    }

    const PairState previous = inMemoryStore[pair];

    // Calculate new Exponential Moving Average
    current.ema = exponentialMovingAverage(current.throughput, emaAlpha, previous.ema);

    // If we have no range, leave it here
    if (range.min == range.max) {
        setOptimizerDecision(pair, range.min, current, 0, "Range fixed", timer.elapsed());
        return true;
    }

    // Apply bandwidth limits (source) for both window based approximation and instantaneous throughput.
    if (limits.throughputSource > 0) {
        if (storageStates[pair.source].asSourceThroughput > limits.throughputSource) {
            decision = std::max(previousValue - decreaseStepSize, range.min);
            rationale << "Source throughput limitation reached (" << limits.throughputSource << ")";
            setOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str(), timer.elapsed());
            return true;
        }
        if (storageStates[pair.source].asSourceThroughputInst > limits.throughputSource) {
            decision = std::max(previousValue - decreaseStepSize, range.min);
            rationale << "Source (instantaneous) throughput limitation reached (" << limits.throughputSource << ")";
            setOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str(), timer.elapsed());
            return true;
        }
    }

    // Apply bandwidth limits (destination)
    if (limits.throughputDestination > 0) {
        if (storageStates[pair.destination].asDestThroughput > limits.throughputDestination) {
            decision = std::max(previousValue - decreaseStepSize, range.min);
            rationale << "Destination throughput limitation reached (" << limits.throughputDestination << ")";
            setOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str(), timer.elapsed());
            return true;
        }
        if (storageStates[pair.destination].asDestThroughputInst > limits.throughputDestination) {
            decision = std::max(previousValue - decreaseStepSize, range.min);
            rationale << "Destination (instantaneous) throughput limitation reached (" << limits.throughputDestination << ")";
            setOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str(), timer.elapsed());
            return true;
        }        
    }

    // Run only when it makes sense
    time_t timeSinceLastUpdate = current.timestamp - previous.timestamp;

    if (current.successRate == previous.successRate &&
        current.ema == previous.ema &&
        timeSinceLastUpdate < optimizerSteadyInterval.total_seconds()) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
            << "Optimizer for " << pair
            << ": Same success rate and throughput EMA, not enough time passed since last update. Skip"
            << commit;
        return false;
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
        << "Optimizer max possible number of actives for " << pair << ": " << range.max << commit;

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
        if (decision > current.queueSize * maxNumberOfStreams) {
            decision = previousValue;
            rationale << ". Too many streams";
        }
    }

    BOOST_ASSERT(decision > 0);
    BOOST_ASSERT(!rationale.str().empty());

    setOptimizerDecision(pair, decision, current, decision - previousValue, rationale.str(), timer.elapsed());
    return true;
}


void Optimizer::setOptimizerDecision(const Pair &pair, int decision, const PairState &current,
    int diff, const std::string &rationale, boost::timer::cpu_times elapsed)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO)
        << "Optimizer: Active for " << pair << " set to " << decision << ", running " << current.activeCount
        << " (" << elapsed.wall << "ns)" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO)
        << rationale << commit;

    inMemoryStore[pair] = current;
    inMemoryStore[pair].connections = decision;
    dataSource->storeOptimizerDecision(pair, decision, current, diff, rationale);

    if (callbacks) {
        callbacks->notifyDecision(pair, decision, current, diff, rationale);
    }
}

}
}
