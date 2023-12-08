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
#include "config/ServerConfig.h"
#include <monitoring/msg-ifce.h>
#include <limits>
#include "Optimizer.h"
#include "OptimizerConstants.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"

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
    if (avgDuration > 0 && avgDuration < 30) {
        return boost::posix_time::minutes(5);
    }
    else if (avgDuration > 30 && avgDuration < 900) {
        return boost::posix_time::minutes(15);
    }
    else {
        return boost::posix_time::minutes(30);
    }
}

// Purpose: Gets and saves current performance on all pairs in 
// memory stores in currentPairStateMap and currentSEStateMap
// Input: List of active pairs, as well as SQL database data.
void Optimizer::getCurrentIntervalInputState(const std::list<Pair> &pairs) {
    // Initializes currentSEStateMap with limit information from
    // t_se table in the SQL database.
    dataSource->getStorageStates(&currentSEStateMap);
    dataSource->getNetLinkStates(&currentNetLinkStateMap); 

    for (auto i = pairs.begin(); i != pairs.end(); ++i) {
        // ===============================================        
        // STEP 1: DERIVING PAIR STATE
        // ===============================================        
       
        // Initialize the PairState object we will store information in
        Pair pair = *i;
        PairState current;

        // Compute the values that will be saved in PairState
        current.timestamp = time(NULL);
        current.avgDuration = dataSource->getAverageDuration(pair, boost::posix_time::minutes(30));
        boost::posix_time::time_duration timeFrame = fts3::optimizer::calculateTimeFrame(current.avgDuration);

        current.activeCount = dataSource->getActive(pair);
        current.successRate = dataSource->getSuccessRateForPair(pair, timeFrame, &current.retryCount);
        current.queueSize = dataSource->getSubmitted(pair);

        // Compute the links associated with the source-destination pair 
        current.netLinks = dataSource->getNetLinks(pair); 

        // Compute throughput values and an estimate of the actual number of connections (since it can deviate from optimizer decision) (used in Step 2)      
        dataSource->getCurrentIntervalTransferInfo(pair, timeFrame, current.activeCount,
          &(current.throughput), &(current.filesizeAvg), &(current.filesizeStdDev), &(current.avgActiveConnections));

        // Save to map
        currentPairStateMap[pair] = current;
        
        // ===============================================        
        // STEP 2: DERIVING SE STATE FROM PAIR STATE
        // ===============================================   

        // Increments SE throughput and total connections value by the pair's throughput and previous optimizer decision value.
        // Because the default and current majority state is no throughput limitation,
        // the if condition is added.
        // Potential difference in list of SEs compared to list of pairs, does not get handled
        if (currentSEStateMap.find(pair.source) != currentSEStateMap.end() 
           && currentSEStateMap[pair.source].outboundMaxThroughput > 0) {
            currentSEStateMap[pair.source].asSourceThroughput += current.throughput;
            currentSEStateMap[pair.source].asSourceNumPairs += 1;
        }

        if (currentSEStateMap.find(pair.destination) != currentSEStateMap.end()
           && currentSEStateMap[pair.destination].inboundMaxThroughput > 0) {
            currentSEStateMap[pair.destination].asDestThroughput += current.throughput;
            currentSEStateMap[pair.destination].asDestNumPairs += 1; 
        }

        // ===============================================        
        // STEP 3: DERIVING LINK STATE FROM PAIR STATE
        // ===============================================
        std::list<std::string> netLinks = currentPairStateMap[pair].netLinks;
        for (const std::string &netLink : netLinks) {
            // Increments link total throughput value by the pair's throughput value 
            if (currentNetLinkStateMap.find(netLink) != currentNetLinkStateMap.end() 
                && currentNetLinkStateMap[netLink].maxThroughput > 0) {
                    currentNetLinkStateMap[netLink].throughput += current.throughput; 
                    currentNetLinkStateMap[netLink].numPairs += 1;
            }
        }         
    }

    //get values of config paramenters
    try {
        windowBasedThroughputLimitEnforcement = fts3::config::ServerConfig::instance().get<bool>("windowBasedThroughputLimitEnforcement");
    }
    catch (...) {
        windowBasedThroughputLimitEnforcement = true;
        FTS3_COMMON_LOGGER_NEWLOG(INFO)
                << "No config entry for windowBasedThroughputLimitEnforcement, defaulting to on" << commit;
    }

    try {
        proportionalDecreaseThroughputLimitEnforcement = fts3::config::ServerConfig::instance().get<bool>("proportionalDecreaseThroughputLimitEnforcement");
    }
    catch (...) {
        proportionalDecreaseThroughputLimitEnforcement = true;
        FTS3_COMMON_LOGGER_NEWLOG(INFO)
                << "No config entry for proportionalDecreaseThroughputLimitEnforcement, defaulting to on" << commit;
    }

    try {
        netLinkThroughputLimitEnforcement = fts3::config::ServerConfig::instance().get<bool>("netLinkThroughputLimitEnforcement");
    }
    catch (...) {
        netLinkThroughputLimitEnforcement = true;
        FTS3_COMMON_LOGGER_NEWLOG(INFO)
                << "No config entry for proportionalDecreaseThroughputLimitEnforcement, defaulting to on" << commit;

    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
                << "window: " << windowBasedThroughputLimitEnforcement << " prop:" << proportionalDecreaseThroughputLimitEnforcement << " netLink: " << netLinkThroughputLimitEnforcement << commit;
}

// Reads limits into Range object for a pair.
// Uses StorageLimits for a hard max.
void Optimizer::getOptimizerWorkingRange(const Pair &pair, const StorageLimits &limits, Range *range)
{
    // Query specific limits
    dataSource->getPairLimits(pair, range);

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
        range->max = std::min({limits.source, limits.destination});
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

// Extracts only the limits from currentSEStateMap
void Optimizer::getStorageLimits(const Pair &pair, StorageLimits *limits) {

    for (const auto& pair : currentSEStateMap) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "SE: " << pair.first << commit;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << pair.second.outboundMaxThroughput << commit;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << pair.second.outboundMaxActive << commit;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << pair.second.inboundMaxThroughput << commit;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << pair.second.inboundMaxActive << commit;
    }

    if (currentSEStateMap.find(pair.source) != currentSEStateMap.end()) {
        limits->throughputSource = currentSEStateMap[pair.source].outboundMaxThroughput;
        limits->source = currentSEStateMap[pair.source].outboundMaxActive;
    }
    else {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG ) << "Source SE not in t_se." << commit;
        limits->throughputSource = currentSEStateMap["*"].outboundMaxThroughput;
        limits->source = currentSEStateMap["*"].outboundMaxActive;
    }
    if (currentSEStateMap.find(pair.destination) != currentSEStateMap.end()) {
        limits->throughputDestination = currentSEStateMap[pair.destination].inboundMaxThroughput;
        limits->destination = currentSEStateMap[pair.destination].inboundMaxActive;          
    }
    else {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG ) << "Destination SE not in t_se." << commit;
        limits->throughputDestination = currentSEStateMap["*"].inboundMaxThroughput;
        limits->destination = currentSEStateMap["*"].inboundMaxActive;        
    }
           
}

// Extracts only the limits from currentLinkStateMap
void Optimizer::getNetLinkLimits(const Pair &pair, std::map<std::string, NetLinkLimits> *limits) {

    std::list<std::string> netLinks = currentPairStateMap[pair].netLinks;

    for (const std::string &netLink : netLinks) {
        if (currentNetLinkStateMap.find(netLink) != currentNetLinkStateMap.end()) {
            (*limits)[netLink].throughput = currentNetLinkStateMap[netLink].maxThroughput;
            (*limits)[netLink].active = currentNetLinkStateMap[netLink].maxActive;
        }
        else {
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG ) << "NetLink is not in t_netLink_stat." << commit;
            (*limits)[netLink].throughput = 0;
            (*limits)[netLink].active = 0; 
        }
    }
}

// Returns the optimizer decision for a given pair if the throughput limits on its storage elements or netlinks are being exceeded 
// If multiple resource limits are being exceeded, returns the minimum optimizer decision to ensure fairness at the pair's bottleneck resource 
int Optimizer::enforceThroughputLimits(const Pair &pair, StorageLimits storageLimits, std::map<std::string, 
                                        NetLinkLimits> netLinkLimits, Range range, int previousValue, int increaseStepSize)
{
    int decision = 0; 
    int minDecision = std::numeric_limits<int>::max();  
    std::stringstream rationale;
    StorageState se; 
    NetLinkState netLinkState;

    if (windowBasedThroughputLimitEnforcement) {
        // Apply bandwidth limits (source) for both window based approximation and instantaneous throughput.
        if (storageLimits.throughputSource > 0) {
            se = currentSEStateMap[pair.source];
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Source SE: " << pair.source \
                    << ", windowBasedThrouput: " << se.asSourceThroughput \
                    << ", inst Throughput:" << se.asSourceThroughputInst << commit;

            if (se.asSourceThroughput > storageLimits.throughputSource) {
                // Check if this has been previously calculated for another pair sharing this resource
                if(proportionalDecreaseThroughputLimitEnforcement) {
                    decision = getFairShareDecision(pair, storageLimits.throughputSource, se.asSourceThroughput, 
                                                    se.asSourceNumPairs, range, previousValue, increaseStepSize);
                }
                else {
                    decision = std::max(previousValue - decreaseStepSize, range.min);
                }

                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Source throughput limit reached " << pair << ": " << storageLimits.throughputSource \
                    << ", Actual window-based throughput on source: " << se.asSourceThroughput \
                    << ", Fair share decision: " << decision << commit;

                rationale << "Source throughput limitation reached (" << storageLimits.throughputSource << ")";
                minDecision = std::min(decision, minDecision); 
            }
            if (se.asSourceThroughputInst > storageLimits.throughputSource) {
                if(proportionalDecreaseThroughputLimitEnforcement) {
                    decision = getFairShareDecision(pair, storageLimits.throughputSource, se.asSourceThroughputInst, 
                                                    se.asSourceNumPairs, range, previousValue, increaseStepSize);
                }
                else {
                    decision = std::max(previousValue - decreaseStepSize, range.min);
                }

                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Source throughput limit reached " << pair << ": " << storageLimits.throughputSource \
                    << ", Actual window-based throughput on source: " << se.asSourceThroughput \
                    << ", Fair share decision: " << decision << commit;
                        
                rationale << "Source (instantaneous) throughput limitation reached (" << storageLimits.throughputSource << ")";
                minDecision = std::min(decision, minDecision); 
            }
        }

        // Apply bandwidth limits (destination) for both window based approximation and instantaneous throughput.
        if (storageLimits.throughputDestination > 0) {

            se = currentSEStateMap[pair.destination];
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Dest SE: " << pair.destination \
                    << ", windowBasedThrouput: " << se.asDestThroughput \
                    << ", inst Throughput:" << se.asDestThroughputInst << commit;
                
            if (se.asDestThroughput > storageLimits.throughputDestination) {
                if(proportionalDecreaseThroughputLimitEnforcement) {
                    decision = getFairShareDecision(pair,storageLimits.throughputDestination, se.asDestThroughput, 
                                                    se.asDestNumPairs, range, previousValue, increaseStepSize);
                }
                else {
                    decision = std::max(previousValue - decreaseStepSize, range.min);
                }

              FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Source throughput limit reached " << pair << ": " << storageLimits.throughputSource \
                    << ", Actual window-based throughput on source: " << se.asSourceThroughput \
                    << ", Fair share decision: " << decision << commit;

                rationale << "Destination throughput limitation reached (" << storageLimits.throughputDestination << ")";
                minDecision = std::min(decision, minDecision); 
            }
            if (se.asDestThroughputInst > storageLimits.throughputDestination) {
                if(proportionalDecreaseThroughputLimitEnforcement) {   
                    decision = getFairShareDecision(pair, storageLimits.throughputDestination, se.asDestThroughputInst, 
                                                    se.asDestNumPairs, range, previousValue, increaseStepSize);
                }
                else {
                    decision = std::max(previousValue - decreaseStepSize, range.min);
                }

                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                    << "S&J: Destination throughput limit reached " << pair << ": " << storageLimits.throughputDestination \
                    << ", Actual instantaneous throughput on destination: " << se.asDestThroughputInst \
                    << ", Fair share decision: " << decision << commit;

                rationale << "Destination (instantaneous) throughput limitation reached (" << storageLimits.throughputDestination << ")";
                minDecision = std::min(decision, minDecision); 
            }        
        }
    }

    if (netLinkThroughputLimitEnforcement) {
        // Apply bandwidth limits (netLinks) for both window based approximation and instantaneous throughput. 
        std::list<std::string> netLinks = currentPairStateMap[pair].netLinks;
        for (const std::string &netLink : netLinks) {
            if (netLinkLimits[netLink].throughput > 0) {
                netLinkState = currentNetLinkStateMap[netLink]; 
                if (netLinkState.throughput > netLinkLimits[netLink].throughput) {
                    if(proportionalDecreaseThroughputLimitEnforcement) {
                        decision = getFairShareDecision(pair, netLinkLimits[netLink].throughput, netLinkState.throughput, 
                                                            netLinkState.numPairs, range, previousValue, increaseStepSize);
                    }
                    else {
                        decision = std::max(previousValue - decreaseStepSize, range.min);
                    }

                    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                        << "S&J: Netlink throughput limit reached on " << netLink << " used by pair " \
                        << pair << ": " << netLinkLimits[netLink].throughput \
                        << ", Actual window-based throughput on netlink: " << netLinkState.throughput \
                        << ", Fair share decision: " << decision << commit;

                    rationale << "Netlink throughput limitation reached (" << netLinkLimits[netLink].throughput << ")";
                    minDecision = std::min(decision, minDecision); 
                }
                if (netLinkState.throughputInst > netLinkLimits[netLink].throughput) {
                    if(proportionalDecreaseThroughputLimitEnforcement) {
                        decision = getFairShareDecision(pair, netLinkLimits[netLink].throughput, netLinkState.throughputInst, 
                                                            netLinkState.numPairs, range, previousValue, increaseStepSize);
                    }
                    else {
                        decision = std::max(previousValue - decreaseStepSize, range.min);
                    }

                    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
                        << "S&J: Netlink throughput limit reached on " << netLink << " used by pair " \
                        << pair << ": " << netLinkLimits[netLink].throughput \
                        << ", Actual instantaneous throughput on netlink: " << netLinkState.throughputInst \
                        << ", Fair share decision: " << decision << commit;

                    rationale << "Netlink (instantaneous) throughput limitation reached (" << netLinkLimits[netLink].throughput << ")";
                    minDecision = std::min(decision, minDecision); 
                }
            }
        }
    }
    // if minDecision has not been modified, the throughput limits are not being exceeded 
    // return 0, and proceed with the optimizer algorithm in optimizeConnectionsForPair to find decision 
    if (minDecision == std::numeric_limits<int>::max()) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
            << "S&J: No resource limits are exceeded for pair " << pair << commit;
        minDecision = 0; 
    }
    else {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
            << "S&J: Resource limits are exceeded for pair " << pair \
            << ", Actual min decision =  " << minDecision << commit;
    }
    return minDecision; 
}

// Returns reduced decision if the throughput limit on the given storage element or netlink is being exceeded 
int Optimizer::getFairShareDecision(const Pair &pair, float tputLimit, float tput, int numPairs, Range range, int previousDecision, int increaseStepSize)
{
    int decision = 0; 

    // To ensure fairness among the competing pairs using the bottlenecked resource, 
    // we want to give each pair an equal amount of throughput 
    float tputPerPair = tputLimit / numPairs; 

    // Calculate each pair's current throughput per connection ratio to account for TCP congestion control 
    // We want to distribute throughput evenly among all pairs competing for the resource 
    // This may result in a different number of connections for each pair, since each pair could have a different (throughput/connections) ratio 
    float pairTputPerConnection;

    if (currentPairStateMap[pair].avgActiveConnections > 0){ //div by 0 check
        pairTputPerConnection = currentPairStateMap[pair].throughput/currentPairStateMap[pair].avgActiveConnections;
    }
    else
    {
        pairTputPerConnection = currentPairStateMap[pair].throughput; //theoretically this should rarely happen
    }
    
    int target = -1;
    if(pairTputPerConnection > 0)
    {
        target = static_cast<int>(tputPerPair/pairTputPerConnection);
    } 

    if(target < 0)
    {
        decision = previousDecision; //no information, keep constant
    }
    else if(target > previousDecision){
        decision = previousDecision + increaseStepSize; // If the allocation of the fair share of throughput causes the decision to increase above the previous value, treat it as an optimizer increase
    }                                   // this is to promote fairness while also making sure we don't inadvertently cause an increase in connections since the scheduler can theoretically start connections faster than it can stop them
    else {
        // Overshoot policy: if decreasing the optimizer decision, we want to overshoot the decrease so that the average 
        // number of connections over the next time interval will be equal to the target decision 
        // since the number of connections will not immediately drop down to the optimizer decision 
        decision = currentPairStateMap[pair].activeCount - 2 * (currentPairStateMap[pair].activeCount - target); 
    }

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
        << "S&J: Proposed fair share target for " << pair << ": " << target \
        << ", Proposed decision (overshoot): " << decision \
        << ", Previous decision: " << previousDecision \
        << ", Throughput per connection: " << pairTputPerConnection \
        << commit;

    decision = std::max(decision, range.min);
    return decision; 
}

// This algorithm idea is similar to the TCP congestion window.
// It gives priority to success rate. If it gets worse, it will back off reducing
// the total number of connections between storages.
// If the success rate is good, and the throughput improves, it will increase the number
// of connections.
// Purpose: Save new decision to database based on network state.
// Return: Boolean signifying whether or not the decision was modified.
bool Optimizer::optimizeConnectionsForPair(OptimizerMode optMode, const Pair &pair)
{
    int decision = 0;
    std::stringstream rationale;

    // Start ticking!
    boost::timer::cpu_timer timer;

    // Reads in range (pair decision's bounds) and limits (total throughut per SE limits)
    Range range;
    StorageLimits storageLimits;
    std::map<std::string, NetLinkLimits> netLinkLimits; 
    
    if(netLinkThroughputLimitEnforcement) {
        getNetLinkLimits(pair, &netLinkLimits);
    }
    getStorageLimits(pair, &storageLimits);
    getOptimizerWorkingRange(pair, storageLimits, &range);
    
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Storage limits for " << pair << ": " << storageLimits.throughputSource
                                    << "/" << storageLimits.throughputDestination
                                    << ", " << storageLimits.source
                                    << "/" << storageLimits.destination << commit;

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Optimizer range for " << pair << ": " << range  << commit;

    // Previous decision 
    int previousValue = dataSource->getOptimizerValue(pair);

    // Read in current pair state information.
    PairState currentPair = currentPairStateMap[pair];

    // Case 1a:
    // There is no value yet. In this case, pick the high value if configured, mid-range otherwise.
    if (previousValue == 0) {
        if (range.specific) {
            decision = range.max;
            rationale << "No information. Use configured range max.";
        } else {
            decision = range.min + (range.max - range.min) / 2;
            rationale << "No information. Start halfway.";
        }

        currentPair.ema = currentPair.throughput; 

        // Update previousPairStateMap variable + SQL tables (t_optimizer and t_optimizer_evolution)
        setOptimizerDecision(pair, decision, currentPair, decision, rationale.str(), timer.elapsed());

        return true;
    }

    // Case 1b: There is information, but it is the first time seen since the restart
    if (previousPairStateMap.find(pair) == previousPairStateMap.end()) {
        currentPair.ema = currentPair.throughput;
        previousPairStateMap[pair] = currentPair;
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Store first feedback from " << pair << commit;
        return false;
    }

    // Read out previous 
    const PairState previous = previousPairStateMap[pair];

    // Calculate new Exponential Moving Average
    currentPair.ema = exponentialMovingAverage(currentPair.throughput, emaAlpha, previous.ema);

    // If we have no range, leave it here
    if (range.min == range.max) {
        setOptimizerDecision(pair, range.min, currentPair, 0, "Range fixed", timer.elapsed());
        return true;
    }
      
    // Check if throughput limits on storage elements on Storage Elements and NetLinks are being respected
    // If not, redistribute the connections and reduce the total connections on the bottlenecked storage element  
    decision = enforceThroughputLimits(pair, storageLimits, netLinkLimits, range, previousValue, increaseStepSize); 
    if (!windowBasedThroughputLimitEnforcement) {
        // Apply bandwidth limits
        if (storageLimits.throughputSource > 0) {
            double throughput = dataSource->getThroughputAsSourceInst(pair.source);
            if (throughput > storageLimits.throughputSource) {
                decision = previousValue - decreaseStepSize;
                rationale << "Source throughput limitation reached (" << storageLimits.throughputSource << ")";
                setOptimizerDecision(pair, decision, currentPair, 0, rationale.str(), timer.elapsed());
                return true;
            }
        }
        if (storageLimits.throughputDestination > 0) {
            double throughput = dataSource->getThroughputAsDestinationInst(pair.destination);
            if (throughput > storageLimits.throughputDestination) {
                decision = previousValue - decreaseStepSize;
                rationale << "Destination throughput limitation reached (" << storageLimits.throughputDestination << ")";
                setOptimizerDecision(pair, decision, currentPair, 0, rationale.str(), timer.elapsed());
                return true;
            }
        }
    }
    if (decision) {
        rationale << "Resource throughput limitation reached";
        setOptimizerDecision(pair, decision, currentPair, decision - previousValue, rationale.str(), timer.elapsed());
        return true; 
    }

    // Run only when it makes sense
    time_t timeSinceLastUpdate = currentPair.timestamp - previous.timestamp;

    if (currentPair.successRate == previous.successRate &&
        currentPair.ema == previous.ema &&
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
    if (currentPair.successRate < lowSuccessRate) {
        // optimizeLowSuccessRate decreases decision (unless success rate is currently increasing)
        decision = optimizeLowSuccessRate(currentPair, previous, previousValue,
            baseSuccessRate, decreaseStepSize,
            rationale);
    }
    // No throughput info
    else if (currentPair.ema == 0) {
        // Does nothing
        decision = optimizeNotEnoughInformation(currentPair, previous, previousValue, rationale);
    }
    // Good success rate, or not enough information to take any decision
    else {
        int localIncreaseStep = increaseStepSize;
        if (optMode >= kOptimizerNormal) {
            localIncreaseStep = increaseAggressiveStepSize;
        }
        // optimizeGoodSuccessRate increases decision if filesize is decreasing or
        // throughput is increasing.
        decision = optimizeGoodSuccessRate(currentPair, previous, previousValue,
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
        if (decision > previousValue && currentPair.queueSize < decision) {
            decision = previousValue;
            rationale << ". Not enough files in the queue";
        }
    }
    // Do not go too far with the number of connections
    if (optMode >= kOptimizerNormal) {
        if (decision > currentPair.queueSize * maxNumberOfStreams) {
            decision = previousValue;
            rationale << ". Too many streams";
        }
    }

    BOOST_ASSERT(decision > 0);
    BOOST_ASSERT(!rationale.str().empty());

    setOptimizerDecision(pair, decision, currentPair, decision - previousValue, rationale.str(), timer.elapsed());
    return true;
}



// setOptimizerDecision does two things
//   - Update Optimizer struct variable (previousPairStateMap)
//   - Update SQL database (t_optimizer and t_optimizer_evolution)
void Optimizer::setOptimizerDecision(const Pair &pair, int decision, const PairState &current,
    int diff, const std::string &rationale, boost::timer::cpu_times elapsed)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO)
        << "Optimizer: Active for " << pair << " set to " << decision << ", running " << current.activeCount
        << " (" << elapsed.wall << "ns)" << commit;

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) \
        << "S&J: Optimizer Decision Info: Pair: " << pair \
        << ", Final Decision: " << decision \
        << ", Running: " << current.activeCount \
        << ", Avg active connections: " << current.avgActiveConnections \
        << ", Queue: " << current.queueSize \
        << ", EMA: " << current.ema \
        << ", Throughput: " << current.throughput \
        << ", Avg filesize: " << current.filesizeAvg \
        << ", Filesize std dev: " << current.filesizeStdDev \
        << ", Success Rate: " << current.successRate \
        << ", diff: " << diff \
        << " (" << elapsed.wall << "ns)" \
        << ", rationale: " << rationale << commit;

    FTS3_COMMON_LOGGER_NEWLOG(INFO)
        << rationale << commit;

    //Stores current PairState (including the optimizer decision) in the previousPairStateMap
    previousPairStateMap[pair] = current;
    previousPairStateMap[pair].optimizerDecision = decision;
    
    dataSource->storeOptimizerDecision(pair, decision, current, diff, rationale);

    if (callbacks) {
        callbacks->notifyDecision(pair, decision, current, diff, rationale);
    }
}

}
}