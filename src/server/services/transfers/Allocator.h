/*
 * Copyright (c) CERN 2013-2015
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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "db/generic/QueueId.h"
#include "db/generic/TransferFile.h"
#include "VoShares.h"

namespace fts3 {
namespace server {

class Allocator
{
public:
    enum AllocatorAlgorithm {
        MAXIMUM_FLOW,
        GREEDY,
    };

    // Function pointer to the allocator algorithm
    using AllocatorFunction = std::map<Pair, int> (*)(std::vector<QueueId>&);

    // Returns function pointer to the allocator algorithm
    static AllocatorFunction getAllocatorFunction();

private:
    // The allocator functions below should execute the corresponding
    // allocator algorithm, and should return a mapping from each VO
    // to the number of slots available on that link

    /**
     * Run greedy allocator algorithm
     * @param queues All current pending transfers
     * @return Number of slots assigned to each link
     */
    static std::map<Pair, int> GreedyAllocator(std::vector<QueueId> &queues);

    /**
     * Run maximum flow allocator algorithm
     * @param queues All current pending transfers
     * @return Number of slots assigned to each link
     */
    static std::map<Pair, int> MaximumFlowAllocator(std::vector<QueueId> &queues);
};

} // end namespace server
} // end namespace fts3

#endif // ALLOCATOR_H