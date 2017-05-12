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
#include "db/generic/QueueId.h"


namespace fts3 {
namespace server {

/**
 * Apply VO shares if required.
 * @param queues Set of queues with queued transfers
 * @return A set of queues where there is only one per unique source/dest.
 *         Filtering is applied according to the configured relative weights.
 */
std::vector<QueueId> applyVoShares(const std::vector<QueueId> queues);

}
}

#endif // VOSHARES_H
