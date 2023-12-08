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

#include "Scheduler.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"

using namespace fts3::common;
using namespace db;

namespace fts3 {
namespace server {

/**
 * Transfers in unschedulable queues must be set to fail
 */
void failUnschedulable(const std::vector<QueueId> &unschedulable, std::map<Pair, int> &slotsPerLink)
{
    Producer producer(config::ServerConfig::instance().get<std::string>("MessagingDirectory"));

    std::map<std::string, std::list<TransferFile> > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getReadyTransfers(unschedulable, voQueues, slotsPerLink);

    for (auto iterList = voQueues.begin(); iterList != voQueues.end(); ++iterList) {
        const std::list<TransferFile> &transferList = iterList->second;
        for (auto iterTransfer = transferList.begin(); iterTransfer != transferList.end(); ++iterTransfer) {
            events::Message status;

            status.set_transfer_status("FAILED");
            status.set_timestamp(millisecondsSinceEpoch());
            status.set_process_id(0);
            status.set_job_id(iterTransfer->jobId);
            status.set_file_id(iterTransfer->fileId);
            status.set_source_se(iterTransfer->sourceSe);
            status.set_dest_se(iterTransfer->destSe);
            status.set_transfer_message("No share configured for this VO");
            status.set_retry(false);
            status.set_errcode(EPERM);

            producer.runProducerStatus(status);
        }
    }
}

Scheduler::SchedulerAlgorithm getSchedulerAlgorithm() {
    std::string schedulerConfig = config::ServerConfig::instance().get<std::string>("TransfersServiceSchedulingAlgorithm");
    if (schedulerConfig == "RANDOMIZED") {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "J&P&C: "
                                        << "Scheduler algorithm: RANDOMIZED"
                                        << commit; 
        return Scheduler::SchedulerAlgorithm::RANDOMIZED;
    }
    else if(schedulerConfig == "DEFICIT") {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "J&P&C: "
                                        << "Scheduler algorithm: DEFICIT"
                                        << commit;
        return Scheduler::SchedulerAlgorithm::DEFICIT;
    }
    else {
        // WARNING log? 
        return Scheduler::SchedulerAlgorithm::RANDOMIZED;
    }
}

Scheduler::SchedulerFunction Scheduler::getSchedulerFunction() {
    Scheduler::SchedulerFunction function;
    switch (getSchedulerAlgorithm()) {
        case Scheduler::RANDOMIZED:
            function = &Scheduler::doRandomizedSchedule;
            break;
        default:
            // Use randomized algorithm as default
            function = &Scheduler::doRandomizedSchedule;
            break;
    }

    return function;
}

std::map<std::string, std::list<TransferFile>> Scheduler::doRandomizedSchedule(
    std::map<Pair, int> &slotsPerLink, 
    std::vector<QueueId> &queues, 
    int availableUrlCopySlots
){
    std::map<std::string, std::list<TransferFile> > scheduledFiles;
    std::vector<QueueId> unschedulable;

    // Apply VO shares at this level. Basically, if more than one VO is used the same link,
    // pick one each time according to their respective weights
    queues = applyVoShares(queues, unschedulable);
    // Fail all that are unschedulable
    failUnschedulable(unschedulable, slotsPerLink);

    if (queues.empty())
        return scheduledFiles;

    auto db = DBSingleton::instance().getDBObjectInstance();

    time_t start = time(0);
    db->getReadyTransfers(queues, scheduledFiles, slotsPerLink); // TODO: move this out of db?
    time_t end =time(0);
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                    << "func=\"doRandomizedSchedule\" "
                                    << "DBcall=\"getReadyTransfers\" " 
                                    << "time=\"" << end - start << "\"" 
                                    << commit;

    return scheduledFiles;
}


} // end namespace server
} // end namespace fts3