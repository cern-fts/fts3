/*
 * Copyright (c) CERN 2017
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

#ifndef VOSHARES_H
#define VOSHARES_H

#include <vector>
#include <db/generic/Pair.h>
#include <map>
#include <db/generic/QueueId.h>
#include <boost/optional.hpp>


namespace fts3 {
namespace server {

/**
 * Apply VO shares if required.
 * @param queues Set of queues with queued transfers
 * @param unschedulable Set of queues that are impossible to schedule, due to empty shares
 * @return A set of queues where there is only one per unique source/dest.
 *         Filtering is applied according to the configured relative weights.
 */
std::vector<QueueId> applyVoShares(const std::vector<QueueId> queues, std::vector<QueueId> &unschedulable);

/**
 * Select a single QueueId for a pair, given a list of waiting VOs and the configured shares
 * @param pair Source => Destination pair
 * @param vos  List of VO waiting for this link
 * @param weights Map with the weights given for the VOs. 'public' is split among those not explictly configured.
 * @param unschedulable If there is no share for a given link/vo combination, it will be put here
 * @return One single link/vo combination, with the probability given by the weights
 */
boost::optional<QueueId> selectQueueForPair(const Pair &pair,
    const std::vector<std::string> &vos,
    const std::map<std::string, double> &weights,
    std::vector<QueueId> &unschedulable);

}
}

#endif // VOSHARES_H
