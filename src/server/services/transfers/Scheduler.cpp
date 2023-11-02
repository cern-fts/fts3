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
    std::map<Pair, int> &slotsPerLink, 
    std::vector<QueueId> &queues, 
    int availableUrlCopySlots
){
    std::map<std::string, std::list<TransferFile> > scheduledFiles;

    if (queues.empty())
        return scheduledFiles;

    // TODO: need to compute the number of active+pending transfers for each activity in each vo
    std::map<std::string, std::map<std::string, int>> voActivityActiveSlots = computeActivePendingCount(std::vector<QueueId> &queues);
    
    // Compute the number of should-be-allocated slots
    std::map<Scheduler::LinkVoActivityKey, int> shouldBeAllocated;
    shouldBeAllocated = computeShouldBeSlots(slotsPerLink, queues);

    // Compute number of active slots for each LinkVoActivityKey

    // Compute deficit for each LinkVoActivityKey

    // Priority queue using deficit

    return scheduledFiles;
}

/**
 * Compute the number of should-be-allocated slots.
*/
std::map<LinkVoActivityKey, int> computeShouldBeSlots(std::map<Pair, int> &slotsPerLink, std::vector<QueueId> &queues)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Scheduler: computing should-be-allocated slots" << commit;

    std::map<LinkVoActivityKey, int> result;

    // For each VO, fetch activity share;
    // we do this first because activity share is independent of link
    std::map<std::string, std::map<std::string, double>> voActivityShare;
    for (auto i = queues.begin(); i != queues.end(); i++) {
        if (voActivityShare.count(i->voName) > 0) {
            // VO already exists in voActivityShare; don't need to fetch again
            continue;
        }
        // Fetch activity share for that VO
        std::map<std::string, double> activityShare = getActivityShareConf(i->voName);
        voActivityShare[i->voName] = activityShare;
    }

    for (auto i = slotsPerLink.begin(); j != slotsPerLink.end(); j++) {
        const Pair &p = i->first;
        const int maxSlots = i->second;

        // (1) Fetch weights of all vo's in that pair
        std::vector<ShareConfig> shares = DBSingleton::instance().getDBObjectInstance()->getShareConfig(p.source, p.destination);
        std::map<std::string, double> voWeights;
        for (auto j = shares.begin(); j != shares.end(); j++) {
            voWeights[j->vo] = j->weight;
        }

        // (2) Assign slots to vo's
        // TODO: this needs to know the number of active + pending transfers for each VO
        std::map<std::string, int> voShouldBeSlots = assignShouldBeSlotsToVos(voWeights, maxSlots);

        // (3) Assign slots of each vo to its activities
        for (auto j = voShouldBeSlots.begin(); j != voShouldBeSlots.end(); j++) {
            std::string voName = j->first;
            int voMaxSlots = j->second;

            std::map<std::string, double> activityWeights = voActivityShare[voName];
            std::map<std::string, int> activityShouldBeSlots = assignShouldBeSlotsToActivities(activityWeights, voMaxSlots);

            // Add to result
            for (auto k = activityShouldBeSlots.begin(); k != activityShouldBeSlots.end(); k++) {
                std::string activityName = k->first;
                int activitySlots = k->second;
                LinkVoActivityKey key = std::make_tuple(p.source, p.destination, voName, activityName);
                result[key] = activitySlots;
            }
        }
    }

    return result;
}

std::map<LinkVoActivityKey, int> computeDeficits(
    std::map<Pair, int> &slotsPerLink, 
    std::vector<QueueId> &queues, 
    int availableUrlCopySlots
){

    // Fetch the number of active slots for each (src, dest, vo, activity)
    auto db = DBSingleton::instance().getDBObjectInstance();
    int activeCount = getActiveCount(sql, it->sourceSe, it->destSe, );

    // need to map each queue to deficit

    // TODO

    return;
}

/**
 * Compute the number of active and pending transfers for each activity in each vo.
*/
std::map<std::string, std::map<std::string, int>> computeActivePendingCount(std::vector<QueueId> &queues)
{
    // For each (src, dest, vo, activity), compute active + pending count
    // TODO

    return;
}

// Helper functions:

/**
 * Transfers in uneschedulable queues must be set to fail
 */
void failUnschedulable(const std::vector<QueueId> &unschedulable)
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