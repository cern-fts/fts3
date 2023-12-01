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

#ifndef FTS3_OPTIMIZER_H
#define FTS3_OPTIMIZER_H

#include <list>
#include <map>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <db/generic/LinkConfig.h>
#include <db/generic/Pair.h>
#include <msg-bus/producer.h>

#include "common/Uri.h"


namespace fts3 {
namespace optimizer {


struct Range {
    int min, max;
    // Set to true if min,max is configured specifically, or is a *->* configuration
    bool specific;
    // Set to true if min,max is configured with SE limits instead of link configuration
    bool storageSpecific;

    Range(): min(0), max(0), specific(false), storageSpecific(false) {}
};


struct StorageLimits {
    int source, destination;
    double throughputSource, throughputDestination;

    StorageLimits(): source(0), destination(0),
              throughputSource(0), throughputDestination(0) {}
};

struct NetLinkLimits {
    int active; // rename to connections
    double throughput; 

    NetLinkLimits(): active(0), throughput(0) {}
};

struct PairState {
    time_t timestamp;
    double throughput;
    time_t avgDuration;
    double successRate;
    int retryCount;
    int activeCount;
    // Average number of concurrent connections in the past time interval 
    // Note: This is based off of the average file count (activeCount) 
    // it is only accurate if there are no streams (one connection = one file)
    int avgActiveConnections; 
    int queueSize;
    // Exponential Moving Average
    double ema;
    // Filesize statistics
    double filesizeAvg, filesizeStdDev;
    // Optimizer last decision
    int optimizerDecision;
    double avgTput;
    // Links in src-dest pair 
    std::list<std::string> netLinks; 
    
    PairState(): timestamp(0), throughput(0), avgDuration(0), successRate(0), retryCount(0), activeCount(0),
                 queueSize(0), ema(0), filesizeAvg(0), filesizeStdDev(0), optimizerDecision(1), avgTput(0), netLinks(0) {}

    PairState(time_t ts, double thr, time_t ad, double sr, int rc, int ac, int qs, double ema, int conn):
        timestamp(ts), throughput(thr), avgDuration(ad), successRate(sr), retryCount(rc),
        activeCount(ac), queueSize(qs), ema(ema), filesizeAvg(0), filesizeStdDev(0), optimizerDecision(conn),
        avgTput(0), netLinks(0) {}
};


struct StorageState {

    //The following two variables store instantaneous inbound (asDest) and outbound (asSource) throughput for a given Storage element.
    //The "Inst" throughput values are calculated by the getThroughputAsSourceInst and getThroughputAsDestinationInst methods (in OptimizerDataSource.cpp)
    //The "Inst" values store throughput based on the number of active transfers at the time the "Inst" methods are called 
    double asSourceThroughputInst;
    double asDestThroughputInst;

    //The following two variables store the window based inbound (asDest) and outbound (asSource) throughput for a given Storage element.
    //These throughput values are calculated in getCurrentIntervalInputState (OptimizerConnections.cpp) by iterating through 
    //all the active pairs and summing the corresponding throughput values returned by getThroughputInfo (OptimizerDataSource.cpp) 
    //for a source-destination pair that involves a given storage element
    double asSourceThroughput;
    double asDestThroughput;

    //The following two variables store the total inbound (asDest) and outbound (asSource) connections for a given Storage elemnt 
    //These values are calculated in getCurrentIntervalInputState (OptimizerConnections.cpp) by iterating through 
    //all the active pairs and summing the corresponding connections based on the pair's current optimizer decision  
    //for a source-destination pair that involves a given storage element
    int asSourceConnections; 
    int asDestConnections;

    //The following two variables store the total inbound (asDest) and outbound (asSource) pairs for a given Storage elemnt 
    //These values are calculated in getCurrentIntervalInputState (OptimizerConnections.cpp) by iterating through 
    //all the active pairs that involves a given storage element
    int asSourceNumPairs;
    int asDestNumPairs; 

    //These values are storage limits for the given storage element
    //They are populated in getStorageStates (OptimizerDataSource.cpp) via querying t_se
    int inboundMaxActive;
    int outboundMaxActive;
    double inboundMaxThroughput;
    double outboundMaxThroughput;
 
    //These variables store the Optimizer decision if the throughput limits are being exceeded for a given Storage Element 
    //They are calculated in enforceThroughputLimits called in optimizeConnectionsForPair (OptimizerConnections.cpp) by
    //redistributing the connections between all the pairs sharing the SE and reducing by a ratio of ThroughputLimit/ActualThroughput. 
    int sourceLimitDecision; 
    int sourceLimitDecisionInst;
    int destLimitDecision; 
    int destLimitDecisionInst;

    StorageState(): asSourceThroughputInst(0), asDestThroughputInst(0),
                    asSourceThroughput(0), asDestThroughput(0), 
                    asSourceConnections(0), asDestConnections(0),
                    asSourceNumPairs(0), asDestNumPairs(0), 
                    inboundMaxActive(0), outboundMaxActive(0),
                    inboundMaxThroughput(0),outboundMaxThroughput(0) {}

    StorageState(int ia, double it, int oa, double ot):
                asSourceThroughputInst(0),asDestThroughputInst(0),
                asSourceThroughput(0), asDestThroughput(0),
                asSourceConnections(0), asDestConnections(0),
                asSourceNumPairs(0), asDestNumPairs(0), 
                inboundMaxActive(ia), outboundMaxActive(oa),
                inboundMaxThroughput(it), outboundMaxThroughput(ot) {}
};	


struct NetLinkState {

    // Stores instantaneous throughput for a given Netlink element 
    // The "Inst" throughput values are calculated by the getThroughputAsLinkInst (in OptimizerDataSource.cpp)
    // The "Inst" values store throughput based on the number of active transfers at the time the "Inst" methods are called 
    double throughputInst;

    // Stores the window based throughput for a given Netlink element 
    // This throughput value is calculated in getCurrentIntervalInputState (OptimizerConnections.cpp) by iterating through 
    // all the active pairs and summing the corresponding throughput and connection values returned by getThroughputInfo (OptimizerDataSource.cpp) 
    // for a source-destination pair that involves a given link element
    double throughput;

    // Stores the total connections and total pairs, respectively, for a given Netlink element 
    // This is calculated in getCurrentIntervalInputState (OptimizerConnections.cpp) by iterating through 
    // all the active pairs and summing the corresponding connections based on the pair's estimated average
    // actual connections for a source-destination pair that involves a given NetLink element 
    int connections;
    int numPairs; 

    // These values are storage the limits for the given Link element 
    // They are populated in getNetLinkStates (OptimizerDataSource.cpp) via querying t_netlink_stat
    int minActive, maxActive;
    double maxThroughput;

    NetLinkState(): throughputInst(0), throughput(0), connections(0), numPairs(0),
                    minActive(0), maxActive(0), maxThroughput(0) {}
    
    NetLinkState(int a, double t):
                throughputInst(0), throughput(0), connections(0), 
                numPairs(0), minActive(0), maxActive(a), maxThroughput(t) {}
};

// To decouple the optimizer core logic from the data storage/representation
class OptimizerDataSource {
public:
    virtual ~OptimizerDataSource()
    {}

    // Return a list of pairs with active or submitted transfers
    virtual std::list<Pair> getActivePairs(void) = 0;

    // Get active throughput and connection limits for every SE  
    virtual void getStorageStates(std::map<std::string, StorageState> *currentSEStateMap) = 0;

    // Get active throughput and connection limits for every link  
    virtual void getNetLinkStates(std::map<std::string, NetLinkState> *currentLinkStateMap) = 0;

    // Return the optimizer configuration value
    virtual OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest) = 0;

    // Get configured storage and pair limits
    virtual void getPairLimits(const Pair &pair, Range *range) = 0;

    // Get the stored optimizer value (current value)
    virtual int getOptimizerValue(const Pair&) = 0;

    // Get the weighted throughput for the pair
    virtual void getThroughputInfo(const Pair &, const boost::posix_time::time_duration &,
        double *throughput, double *filesizeAvg, double *filesizeStdDev) = 0;

    virtual time_t getAverageDuration(const Pair&, const boost::posix_time::time_duration&) = 0;

    // Get the success rate for the pair
    virtual double getSuccessRateForPair(const Pair&, const boost::posix_time::time_duration&, int *retryCount) = 0;

    // Get the number of transfers in the given state
    virtual int getActive(const Pair&) = 0;
    virtual int getSubmitted(const Pair&) = 0;

    // Get the bottleneck link (smallest throughput capacity)
    virtual std::list<std::string> getNetLinks(const Pair&) = 0;

    // Get current throughput
    virtual double getThroughputAsSourceInst(const std::string&) = 0;
    virtual double getThroughputAsDestinationInst(const std::string&) = 0;
    virtual double getThroughputOverNetLinkInst(const std::string&) = 0;

    // Permanently register the optimizer decision
    virtual void storeOptimizerDecision(const Pair &pair, int activeDecision,
        const PairState &newState, int diff, const std::string &rationale) = 0;

    // Permanently register the number of streams per active
    virtual void storeOptimizerStreams(const Pair &pair, int streams) = 0;
};

// Used by the optimizer to notify decisions
class OptimizerCallbacks {
public:
    virtual void notifyDecision(const Pair &pair, int decision, const PairState &current,
        int diff, const std::string &rationale) = 0;
};

/*
 * Conceptually the optimizer is a dynamic system, of the form
 * y(n) = f( x(n-1), x(n), y(n-1) ),
 * where n: current interval
 *       x(n): input state
 *       y(n): persistent, decision state
 * 
 * Note that the input state can be broken down into three components
 *     x1(n): is local and will be used only by a pair
 *     x2(n): is also local, but will be needed to compute x3
 *     x3(n): more global state and depends on the sum of x2(n)
 * 
 * At the moment, the optimizer has the following structure 
 *     y(n) = f( x1(n), x2(n), x3(n), y(n-1))
 * 
 * But for sake of clarity, we save the entire vectors:
 *     x_{1,2}(n-1) in previousPairStateMap
 *     x_{1,2}(n) in currentPairStateMap
 *     x_{3}(n) in currentSEStateMap
*/
// Optimizer implementation
class Optimizer: public boost::noncopyable {
protected:
    std::map<Pair, PairState> previousPairStateMap;
    std::map<Pair, PairState> currentPairStateMap;
    std::map<std::string, StorageState> currentSEStateMap;
    std::map<std::string, NetLinkState> currentNetLinkStateMap; 

    OptimizerDataSource *dataSource;
    OptimizerCallbacks *callbacks;
    boost::posix_time::time_duration optimizerSteadyInterval;
    int maxNumberOfStreams;
    int maxSuccessRate;
    int lowSuccessRate;
    int baseSuccessRate;

    int decreaseStepSize;
    int increaseStepSize, increaseAggressiveStepSize;
    double emaAlpha;

    double defaultNetLinkMaxThroughput;
    int defaultNetLinkMaxActive;

    bool windowBasedThroughputLimitEnforcement;
    bool netLinkThroughputLimitEnforcement;
    bool proportionalDecreaseThroughputLimitEnforcement;

    // Read currentSEStateMap values into a StorageLimits object for the purposes of a single pair.
    void getStorageLimits(const Pair &pair, StorageLimits *limits);

    // Read currentLinkStateMap values into a NetLinkLimits object for the purposes of a single pair.
    void getNetLinkLimits(const Pair &pair, std::map<std::string, NetLinkLimits> *limits);

    // Enforce throughput limits on storage elements and netlinks used by a given pair. 
    // Returns reduced optimizer decision for given pair if throughput limit is exceeded. 
    int enforceThroughputLimits(const Pair &pair, StorageLimits storageLimits, std::map<std::string, NetLinkLimits> netLinkLimits, Range range, int previousValue);

    // Calculates the reduced optimizer decision if throughput limits on storage element or netlinks are exceeded 
    int getReducedDecision(const Pair &pair, float tputLimit, float tput, int connections, int numPairs, Range range);

    // Run the optimization algorithm for the number of connections.
    // Returns true if a decision is stored
    bool optimizeConnectionsForPair(OptimizerMode optMode, const Pair &);

    // Run the optimization algorithm for the number of streams.
    void optimizeStreamsForPair(OptimizerMode optMode, const Pair &);

    // Stores into rangeActiveMin and rangeActiveMax the working range for the optimizer
    void getOptimizerWorkingRange(const Pair &pair, const StorageLimits &limits, Range *range);

    // Updates decision
    void setOptimizerDecision(const Pair &pair, int decision, const PairState &current,
        int diff, const std::string &rationale, boost::timer::cpu_times elapsed);

    // Gets and saves current performance on all pairs and storage elements 
    // in currentPairStateMap and currentSEStateMap
    int getAvgActiveConnections(const Pair &pair);

    // Gets and saves current performance on all pairs and storage elements 
    // in currentPairStateMap and currentSEStateMap
    void getCurrentIntervalInputState(const std::list<Pair> &);

public:
    Optimizer(OptimizerDataSource *ds, OptimizerCallbacks *callbacks);
    ~Optimizer();

    // Calculates the ideal window size for throughput estimation
    boost::posix_time::time_duration calculateTimeFrame(time_t avgDuration);

    void setSteadyInterval(boost::posix_time::time_duration);
    void setMaxNumberOfStreams(int);
    void setMaxSuccessRate(int);
    void setLowSuccessRate(int);
    void setBaseSuccessRate(int);
    void setStepSize(int increase, int increaseAggressive, int decrease);
    void setEmaAlpha(double);
    void updateDecisions(const std::list<Pair> &);
    void run(void);
    void runOptimizerForPair(const Pair&);
};


inline std::ostream& operator << (std::ostream &os, const Range &range) {
    return (os << range.min << "/" << range.max);
}

}
}

#endif // FTS3_OPTIMIZER_H