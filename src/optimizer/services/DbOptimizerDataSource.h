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

#include "db/generic/SingleDbInstance.h"
#include "OptimizerDataSource.h"

using namespace db;

namespace fts3 {
namespace optimizer {

    class DbOptimizerDataSource : public OptimizerDataSource {

    public:
        // DB interface
        GenericDbIfce *db;

        DbOptimizerDataSource() : db(DBSingleton::instance().getDBObjectInstance()) {
        }

        ~DbOptimizerDataSource() override = default;

        OptimizerMode getOptimizerMode(const std::string &source, const std::string &dest) override;

        // Get configured limits
        void getPairLimits(const Pair &pair, Range &range, StorageLimits &limits) override;

        // Get the stored optimizer value (current value)
        int getOptimizerValue(const Pair &pair) override;

        // Get the weighted throughput for the pair
        void getThroughputInfo(const Pair &pair, const boost::posix_time::time_duration &interval,
                               double *throughput, double *filesizeAvg, double *filesizeStdDev) override;

        time_t getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval) override;

        // Get the success rate for the pair
        double getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval, int *retryCount) override;

        // Get the number of transfers in active state
        int getActive(const Pair &pair) override;

        // Get the number of transfers in submitted state
        int getSubmitted(const Pair &pair) override;

        // Get current throughput
        double getThroughputAsSource(const std::string &se) override;

        // Get current throughput
        double getThroughputAsDestination(const std::string &se) override;

        // Permanently register the optimizer decision
        void storeOptimizerDecision(const Pair &pair, int activeDecision,
                                    const PairState &newState, int diff, const std::string &rationale) override;

        // Permanently register the number of streams per active
        void storeOptimizerStreams(const Pair &pair, int streams) override;

    };
}
}
