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

#include "db/generic/QueueId.h"
#include "db/generic/TransferFile.h"
#include "VoShares.h"

#ifndef SCHEDULER_H_
#define SCHEDULER_H_
namespace fts3 {
namespace server {

class Scheduler
{
public:
    enum SchedulerAlgorithm {
        RANDOMIZED,
        DEFICIT
    };
    // Define LinkVoActivityKey, which is (src, dest, vo, activity)
    using LinkVoActivityKey = std::tuple<std::string, std::string, std::string, std::string>;

    // Function pointer to the scheduler algorithm
    using SchedulerFunction = std::map<std::string, std::list<TransferFile>> (*)(std::map<Pair, int>&, std::vector<QueueId>&, int);

    // Returns function pointer to the scheduler algorithm
    static SchedulerFunction getSchedulerFunction();

private:
    // The scheduling functions below should execute the corresponding
    // scheduling algorithm, and they should return a mapping from
    // VOs to the list of TransferFiles to be scheduled by TransferService.
    //
    // Note that the list of TransferFiles returned here has not yet accounted
    // for the number of slots available for src/dest nodes. This will happen
    // in TransferService.

    /**
     * Run scheduling using weighted randomization.
     * @param slotsPerLink number of slots assigned to each link, as determined by allocator
     * @param queues All current pending transfers
     * @param availableUrlCopySlots Max number of slots available in the system
     * @return Mapping from each VO to the list of transfers to be scheduled.
     */
    static std::map<std::string, std::list<TransferFile>> doRandomizedSchedule(std::map<Pair, int> &slotsPerLink, std::vector<QueueId> &queues, int availableUrlCopySlots);

    /**
     * Run deficit-based priority queueing scheduling.
     * @param slotsPerLink number of slots assigned to each link, as determined by allocator
     * @param queues All current pending transfers
     * @param availableUrlCopySlots Max number of slots available in the system
     * @return Mapping from each VO to the list of transfers to be scheduled.
     */
    static std::map<std::string, std::list<TransferFile>> doDeficitSchedule(std::map<Pair, int> &slotsPerLink, std::vector<QueueId> &queues, int availableUrlCopySlots);
};


} // end namespace server
} // end namespace fts3

#endif // DAEMONTOOLS_H_
