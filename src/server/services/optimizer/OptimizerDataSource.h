/*
 * Copyright (c) CERN 2024
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

#pragma once

#include <list>
#include <map>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/timer/timer.hpp>

#include <db/generic/Pair.h>
#include <db/generic/LinkConfig.h>

namespace fts3 {
namespace optimizer {

// To decouple the optimizer core logic from the data storage/representation
class OptimizerDataSource {
public:
    virtual ~OptimizerDataSource() {}

    // Return the optimizer configuration value
    virtual OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest) = 0;

    // Get configured limits
    virtual void getPairLimits(const Pair &pair, Range &range, StorageLimits &limits) = 0;

    // Get the stored optimizer value (current value)
    virtual int getOptimizerValue(const Pair &) = 0;

    // Get the weighted throughput for the pair
    virtual void getThroughputInfo(const Pair &, const boost::posix_time::time_duration &,
                                   double *throughput, double *filesizeAvg, double *filesizeStdDev) = 0;

    virtual time_t getAverageDuration(const Pair &, const boost::posix_time::time_duration &) = 0;

    // Get the success rate for the pair
    virtual double
    getSuccessRateForPair(const Pair &, const boost::posix_time::time_duration &, int *retryCount) = 0;

    // Get the number of transfers in active state
    virtual int getActive(const Pair &) = 0;

    // Get the number of transfers in submitted state
    virtual int getSubmitted(const Pair &) = 0;

    // Get current throughput
    virtual double getThroughputAsSource(const std::string &) = 0;

    // Get current throughput
    virtual double getThroughputAsDestination(const std::string &) = 0;

    // Permanently register the optimizer decision
    virtual void storeOptimizerDecision(const Pair &pair, int activeDecision,
                                        const PairState &newState, int diff, const std::string &rationale) = 0;

    // Permanently register the number of streams per active
    virtual void storeOptimizerStreams(const Pair &pair, int streams) = 0;
};

}
}
