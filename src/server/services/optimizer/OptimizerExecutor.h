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

#ifndef FTS3_OPTIMIZEREXECUTOR_H
#define FTS3_OPTIMIZEREXECUTOR_H

#include <list>
#include <map>
#include <string>
#include <memory>

#include <boost/noncopyable.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/any.hpp>
#include <db/generic/LinkConfig.h>
#include <db/generic/Pair.h>
#include "db/generic/SingleDbInstance.h"
#include <msg-bus/producer.h>

#include "common/Uri.h"
#include "OptimizerDataSource.h"
#include "OptimizerCallbacks.h"

namespace fts3 {
namespace optimizer {

// Generic optimizer implementation
class OptimizerExecutor {
protected:
    std::map<Pair, PairState> inMemoryStore;
    // DB interface
    std::unique_ptr<OptimizerDataSource> dataSource;
    std::unique_ptr<OptimizerCallbacks> callbacks;
    boost::posix_time::time_duration optimizerSteadyInterval;
    int maxNumberOfStreams;
    int maxSuccessRate;
    int lowSuccessRate;
    int baseSuccessRate;

    int decreaseStepSize;
    int increaseStepSize, increaseAggressiveStepSize;
    double emaAlpha;

    size_t pairsSize; /// Number of pairs on Optimizer run
    int pairIdx; /// Current pair index
    Pair pair; /// Current pair being optimized

    // Run the optimization algorithm for the number of connections.
    // Returns true if a decision is stored
    bool optimizeConnectionsForPair(OptimizerMode optMode);

    // Run the optimization algorithm for the number of streams.
    void optimizeStreamsForPair(OptimizerMode optMode);

    // Stores into rangeActiveMin and rangeActiveMax the working range for the optimizer
    void getOptimizerWorkingRange(Range& range, StorageLimits& limits);

    // Updates decision
    void setOptimizerDecision(int decision, const PairState &current, int diff,
                              const std::string &rationale, boost::timer::cpu_times elapsed);

public:
    OptimizerExecutor(std::unique_ptr<OptimizerDataSource> ds, std::unique_ptr<OptimizerCallbacks> callbacks, const Pair& pair);
    ~OptimizerExecutor();

    void setSteadyInterval(boost::posix_time::time_duration);
    void setMaxNumberOfStreams(int);
    void setMaxSuccessRate(int);
    void setLowSuccessRate(int);
    void setBaseSuccessRate(int);
    void setStepSize(int increase, int increaseAggressive, int decrease);
    void setEmaAlpha(double);
    void run(boost::any &);
    void runOptimizerForPair();
};

}
}

#endif // FTS3_OPTIMIZEREXECUTOR_H
