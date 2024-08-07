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

#include <map>
#include <string>
#include <memory>
#include <chrono>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>
#include <boost/any.hpp>
#include <db/generic/LinkConfig.h>
#include <db/generic/Pair.h>

#include "OptimizerDataSource.h"
#include "OptimizerCallbacks.h"

namespace fts3 {
namespace optimizer {

class OptimizerExecutor {
public:
    OptimizerExecutor(std::unique_ptr<OptimizerDataSource> ds, std::unique_ptr<OptimizerCallbacks> callbacks, const Pair& pair);
    ~OptimizerExecutor() = default;

    void run(boost::any &);
    void runOptimizerForPair();

    void setSteadyInterval(const boost::posix_time::time_duration& steadyInterval) {
        optimizerSteadyInterval = steadyInterval;
    }

    void setMaxNumberOfStreams(const int numberOfStreams) {
        maxNumberOfStreams = numberOfStreams;
    }

    void setMaxSuccessRate(const int successRate) {
        maxSuccessRate = successRate;
    }

    void setLowSuccessRate(const int successRate) {
        lowSuccessRate = successRate;
    }

    void setBaseSuccessRate(const int successRate) {
        baseSuccessRate = successRate;
    }

    void setStepSize(const int increase, const int increaseAggressive, const int decrease) {
        increaseStepSize = increase;
        increaseAggressiveStepSize = increaseAggressive;
        decreaseStepSize = decrease;
    }

    void setEmaAlpha(const double alpha) {
        emaAlpha = alpha;
    }

protected:
    // Run the optimization algorithm for the number of connections.
    // Returns true if a decision is stored
    bool optimizeConnectionsForPair(OptimizerMode optMode);

    // Run the optimization algorithm for the number of streams.
    void optimizeStreamsForPair(OptimizerMode optMode);

    // Stores into rangeActiveMin and rangeActiveMax the working range for the optimizer
    void getOptimizerWorkingRange(Range& range, StorageLimits& limits);

    // Updates decision
    void setOptimizerDecision(const PairState& current, int decision, int diff,
                              const std::string& rationale,
                              const std::chrono::steady_clock::time_point& start);

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
    int increaseStepSize;
    int increaseAggressiveStepSize;
    double emaAlpha;

    Pair pair; /// The pair being optimized
};

}
}

#endif // FTS3_OPTIMIZEREXECUTOR_H
