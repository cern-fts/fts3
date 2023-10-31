#include "Scheduler.h"

using namespace fts3::common;

namespace fts3 {
namespace server {

Scheduler::SchedulerFunction Scheduler::getSchedulingFunction(string algorithm) {
    Scheduler::SchedulerFunction function;

    switch (algorithm) {
        case Scheduler::RANDOMIZED_ALGORITHM:
            function = &Scheduler::doRandomizedSchedule;
            break;
        case Scheduler::DEFICIT_ALGORITHM:
            function = &Scheduler::doDeficitSchedule;
            break;
        case default:
            // Use randomized algorithm as default
            function = &Scheduler::doRandomizedSchedule;
            break;
    }

    return function;
}

/**
 * @param queues All pending queues.
*/
std::map<std::string, std::list<TransferFile>> Scheduler::doRandomizedSchedule(
    std::vector<int> slotsPerLink, 
    std::vector<QueueId> queues, 
    int availableUrlCopySlots)
{
    std::map<std::string, std::list<TransferFile> > scheduledFiles;
    std::vector<QueueId> unschedulable;

    // Apply VO shares at this level. Basically, if more than one VO is used the same link,
    // pick one each time according to their respective weights
    queues = applyVoShares(queues, unschedulable);
    // Fail all that are unschedulable
    failUnschedulable(unschedulable);

    if (queues.empty())
        return scheduledFiles;

    auto db = DBSingleton::instance().getDBObjectInstance();

    time_t start = time(0);
    db->getReadyTransfers(queues, scheduledFiles); // TODO: move this out of db?
    time_t end =time(0);
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                    << "func=\"doRandomizedSchedule\" "
                                    << "DBcall=\"getReadyTransfers\" " 
                                    << "time=\"" << end - start << "\"" 
                                    << commit;

    if (voQueues.empty())
        return;

    return scheduledFiles;
}

std::map<std::string, std::list<TransferFile>> Scheduler::doDeficitSchedule(
    std::vector<int> slotsPerLink, 
    std::vector<QueueId> queues, 
    int availableUrlCopySlots)
{
    std::map<std::string, std::list<TransferFile> > scheduledFiles;

    if (queues.empty())
        return scheduledFiles;

    // TODO

    return scheduledFiles;
}

// Helper functions:

/**
 * Transfers in uneschedulable queues must be set to fail
 */
static void failUnschedulable(const std::vector<QueueId> &unschedulable)
{
    Producer producer(config::ServerConfig::instance().get<std::string>("MessagingDirectory"));

    std::map<std::string, std::list<TransferFile> > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getReadyTransfers(unschedulable, voQueues);

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

} // end namespace server
} // end namespace fts3