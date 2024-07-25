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

#include "db/generic/SingleDbInstance.h"
#include "DbOptimizerDataSource.h"

using namespace fts3;
using namespace optimizer;


OptimizerMode DbOptimizerDataSource::getOptimizerMode(const std::string &source, const std::string &dest) {
    return db->getOptimizerMode(source, dest);
}

void DbOptimizerDataSource::getPairLimits(const Pair &pair, Range &range, StorageLimits &limits) {
    return db->getPairLimits(pair, range, limits);
}

int DbOptimizerDataSource::getOptimizerValue(const Pair &pair) {
    return db->getOptimizerValue(pair);
}

void DbOptimizerDataSource::getThroughputInfo(const Pair &pair, const boost::posix_time::time_duration &interval,
                       double *throughput, double *filesizeAvg, double *filesizeStdDev) {
    return db->getThroughputInfo(pair, interval, throughput, filesizeAvg, filesizeStdDev);
}

time_t DbOptimizerDataSource::getAverageDuration(const Pair &pair, const boost::posix_time::time_duration &interval) {
    return db->getAverageDuration(pair, interval);
}

double DbOptimizerDataSource::getSuccessRateForPair(const Pair &pair, const boost::posix_time::time_duration &interval, int *retryCount) {
    return db->getSuccessRateForPair(pair, interval, retryCount);
}

int DbOptimizerDataSource::getActive(const Pair &pair) {
    return db->getCountInState(pair, "ACTIVE");
}

int DbOptimizerDataSource::getSubmitted(const Pair &pair) {
    return db->getCountInState(pair, "SUBMITTED");
}

double DbOptimizerDataSource::getThroughputAsSource(const std::string &se) {
    return db->getThroughputAsSource(se);
}

double DbOptimizerDataSource::getThroughputAsDestination(const std::string &se) {
    return db->getThroughputAsDestination(se);
}

void DbOptimizerDataSource::storeOptimizerDecision(const Pair &pair, int activeDecision,
                            const PairState &newState, int diff, const std::string &rationale) {
    db->storeOptimizerDecision(pair, activeDecision, newState, diff, rationale);
}

void DbOptimizerDataSource::storeOptimizerStreams(const Pair &pair, int streams) {
    return db->storeOptimizerStreams(pair, streams);
}
