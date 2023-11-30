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

Scheduler::SchedulerAlgorithm getSchedulerAlgorithm() {
    std::string schedulerConfig = config::ServerConfig::instance().get<std::string>("TransfersServiceSchedulingAlgorithm");
    if (schedulerConfig == "RANDOMIZED") {
        return Scheduler::SchedulerAlgorithm::RANDOMIZED;
    }
    else if(schedulerConfig == "GREEDY") {
        return Scheduler::SchedulerAlgorithm::DEFICIT;
    }
    else {
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

    // (1) For each VO, fetch activity share.
    //     We do this here because this produces a mapping between each vo and all activities in that vo,
    //     which we need for later steps. Hence this reduces redundant queries into the database.
    //     The activity share of each VO also does not depend on the pair.
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
    
    std::map<Scheduler::LinkVoActivityKey, double> allDeficits;

    for (auto i = slotsPerLink.begin(); i != slotsPerLink.end(); i++) {
        const Pair &p = i->first;
        const int maxSlots = i->second;

        // (2) Compute the number of active / submitted transfers for each activity in each vo,
        //     as well as the number of pending (submitted) transfers for each activity in each vo.
        //     We do this here because we need this for computing should-be-allocated slots.
        std::map<std::string, std::map<std::string, int>> queueActiveCounts = computeActiveCounts(p->source, p->destination, voActivityShare);
        std::map<std::string, std::map<std::string, int>> queueSubmittedCounts = computeSubmittedCounts(p->source, p->destination, voActivityShare);

        // (3) Compute the number of should-be-allocated slots.
        std::map<std::string, std::map<std::string, int>> queueShouldBeAllocated = computeShouldBeSlots(
            p, maxSlots,
            voActivityShare,
            queueActiveCounts,
            queueSubmittedCounts);

        // (4) Compute deficit.
        std::map<std::string, std::map<std::string, int>> deficits = computeDeficits(queueActiveCounts, queueShouldBeAllocated);

        // (5) Store deficit into allDeficits.
        for (auto j = deficits.begin(); j != deficits.end(); j++) {
            std::string voName = j->first;
            std::map<std::string, int> activityDeficits = j->second;
            for (auto k = activityDeficits.begin(); k != deficits.end(); k++) {
                std::string activityName = k->first;
                int deficit = k->second;

                LinkVoActivityKey key = std::make_tuple(p->source, p->destination, voName, activityName);
                allDeficits[key] = deficit;
            }
        }
    }

    // (6) Schedule using priority queue on deficits.
    scheduledFiles = scheduleUsingDeficit(allDefits);

    return scheduledFiles;
}

/**
 * Compute the number of active transfers for each activity in each vo for the pair.
 * @param src Source node
 * @param dest Destination node
 * @param voActivityShare Maps each VO to a mapping between each of its activities to the activity's weight
*/
std::map<std::string, std::map<std::string, long long>> computeActiveCounts(
    std::string src,
    std::string dest,
    std::map<std::string, std::map<std::string, double>> &voActivityShare
){
    auto db = DBSingleton::instance().getDBObjectInstance(); // TODO: fix repeated db
    std::map<std::string, std::map<std::string, long long>> result;

    for (auto i = voActivityShare.begin(); i != voActivityShare.end(); i++) {
        std::string voName = i->first;
        result[voName] = db->getActiveCountInActivity(src, dest, vo);
    }

    return result;
}

/**
 * Compute the number of submitted transfers for each activity in each vo for the pair.
 * @param src Source node
 * @param dest Destination node
 * @param voActivityShare Maps each VO to a mapping between each of its activities to the activity's weight
*/
std::map<std::string, std::map<std::string, long long>> computeSubmittedCounts(
    std::string src,
    std::string dest,
    std::map<std::string, std::map<std::string, double>> &voActivityShare
){
    auto db = DBSingleton::instance().getDBObjectInstance(); // TODO: fix repeated db
    std::map<std::string, std::map<std::string, long long>> result;

    for (auto i = voActivityShare.begin(); i != voActivityShare.end(); i++) {
        std::string voName = i->first;
        result[voName] = db->getSubmittedCountInActivity(src, dest, vo);
    }

    return result;
}

/**
 * Compute the number of should-be-allocated slots.
 * @param p Pair of src-dest nodes.
 * @param maxPairSlots Max number of slots given to the pair, as determined by allocator.
 * @param voActivityShare Maps each VO to a mapping between each of its activities to the activity's weight.
 * @param queueActiveCounts Maps each VO to a mapping between each of its activities to the activity's number of active slots.
 * @param queueSubmittedCounts Maps each VO to a mapping between each of its activities to the activity's number of submitted slots.
*/
std::map<std::string, std::map<std::string, int>> computeShouldBeSlots(
    Pair &p,
    int maxPairSlots,
    std::map<std::string, std::map<std::string, double>> &voActivityShare,
    std::map<std::string, std::map<std::string, int>> &queueActiveCounts,
    std::map<std::string, std::map<std::string, int>> &queueSubmittedCounts
){
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Scheduler: computing should-be-allocated slots" << commit;

    std::map<std::string, std::map<std::string, int>> result;

    // (1) Fetch weights of all vo's in that pair
    std::vector<ShareConfig> shares = DBSingleton::instance().getDBObjectInstance()->getShareConfig(p.source, p.destination);
    std::map<std::string, double> voWeights;
    for (auto j = shares.begin(); j != shares.end(); j++) {
        voWeights[j->vo] = j->weight;
    }

    // (2) Assign slots to vo's
    // TODO: this needs to know the number of active + pending transfers for each VO
    std::map<std::string, int> voShouldBeSlots = assignShouldBeSlotsToVos(voWeights, maxPairSlots, queueActiveCounts, queueSubmittedCounts);

    // (3) Assign slots of each vo to its activities
    for (auto j = voShouldBeSlots.begin(); j != voShouldBeSlots.end(); j++) {
        std::string voName = j->first;
        int voMaxSlots = j->second;

        std::map<std::string, double> activityWeights = voActivityShare[voName];
        std::map<std::string, int> activityShouldBeSlots = assignShouldBeSlotsToActivities(activityWeights, voMaxSlots, queueActiveCounts, queueSubmittedCounts);

        // Add to result
        for (auto k = activityShouldBeSlots.begin(); k != activityShouldBeSlots.end(); k++) {
            std::string activityName = k->first;
            int activitySlots = k->second;

            result[voName][activityName] = activitySlots;
        }
    }

    return result;
}

/**
 * Assign should-be-allocated slots to each VO, using Huntington-Hill algorithm.
 * @param voWeights Weight of each VO.
 * @param maxPairSlots Max number of slots to be allocated to the VOs.
 * @param queueActiveCounts Number of active transfers associated with each VO and each activity in the VO.
 * @param queueSubmittedCounts Number of submitted transfers associated with each VO and each activity in the VO.
*/
std::map<std::string, int> assignShouldBeSlotsToVos(
    std::map<std::string, double> &voWeights,
    int maxPairSlots,
    std::map<std::string, std::map<std::string, int>> &queueActiveCounts,
    std::map<std::string, std::map<std::string, int>> &queueSubmittedCounts
){
    // Compute total number of active and pending transfers for each vo
    std::map<std::string, int> activeAndPendingCounts;

    for (auto i = voWeights.begin(); i != voWeights.end(); i++) {
        std::string voName = i->first;
        double voWeight = i->second;

        activeAndPendingCounts[voName] = 0;

        if (queueActiveCounts.count(voName) > 0) {
            std::map<std::string, int>> activityActiveCounts = queueActiveCounts[voName];
            for (auto j = activityActiveCounts.begin(); j != activityActiveCounts.end(); j++) {
                activeAndPendingCounts[voName] += j->second;
            }
        }
        if (queueSubmittedCounts.count(voName) > 0) {
            std::map<std::string, int>> activitySubmittedCounts = queueSubmittedCounts[voName];
            for (auto j = activitySubmittedCounts.begin(); j != activitySubmittedCounts.end(); j++) {
                activeAndPendingCounts[voName] += j->second;
            }
        }
    }

    return huntingtonHill(voWeights, maxPairSlots, activeAndPendingCounts);
}

/**
 * Assign should-be-allocated slots to each activity, using Huntington-Hill algorithm.
 * @param activityWeights Weight of each activity.
 * @param voMaxSlots Max number of slots to be allocated to the activities.
 * @param activityActiveCounts Number of active transfers associated with each activity.
 * @param activitySubmittedCounts Number of submitted transfers associated with each activity.
*/
std::map<std::string, int> assignShouldBeSlotsToActivities(
    std::map<std::string, double> &activityWeights,
    int voMaxSlots,
    std::map<std::string, int> &activityActiveCounts,
    std::map<std::string, int> &activitySubmittedCounts
){
    // Compute total number of active and pending transfers for each activity
    std::map<std::string, int> activeAndPendingCounts;

    for (auto i = activityWeights.begin(); i != activityWeights.end(); i++) {
        std::string activityName = i->first;
        double activityWeight = i->second;

        activeAndPendingCounts[activityName] = 0;

        if (activityActiveCounts.count(activityName) > 0) {
            activeAndPendingCounts[activityName] += activityActiveCounts[activityName];
        }
        if (activitySubmittedCounts.count(activityName) > 0) {
            activeAndPendingCounts[activityName] += activitySubmittedCounts[activityName];
        }
    }

    return huntingtonHill(activityWeights, voMaxSlots, activeAndPendingCounts);
}

/**
 * Assign slots to the VOs/activities via the Huntington-Hill algorithm.
 * (Both VO and activity will be referred to as queue here, because this function will be used for both).
 * @param weights Maps each queue name to the respective weight.
 * @param maxSlots Max number of slots to be allocated.
 * @param activeAndPendingCounts Number of active or pending transfers for each queue.
*/
std::map<std::string, int> huntingtonHill(
    std::map<std::string, double> &weights,
    int maxSlots,
    std::map<std::string, int> &activeAndPendingCounts
){
    std::map<std::string, int> allocation;

    // Default all queues to 0 in allocation
    for (auto i = weights.begin(); i != weights.end(); i++) {
        std::string queueName = i->first;
        allocation[queueName] = 0;
    }

    if (maxSlots == 0) {
        return allocation;
    }

    // Compute qualification threshold;
    // this step only includes non-empty queues (with either active or pending transfers).
    double weightSum = 0
    for (auto i = activeAndPendingCounts.begin(); i != activeAndPendingCounts.end(); i++) {
        std::string queueName = i->first;
        double count = i->second;
        if (count > 0) {
            weightSum += weights[queueName];
        }
    }
    double threshold = weightSum / maxSlots;

    // Assign one slot to every queue that meets the threshold; compute A_{1}
    std::priority_queue<std::tuple<double, string>> pq;

    for (auto i = weights.begin(); i != weights.end(); i++) {
        std::string queueName = i->first;
        double weight = i->second;

        if (activeAndPendingCounts[queueName] > 0 && weight >= threshold) {
            allocation[queueName] = 1
            maxSlots -= 1

            // Compute priority
            priority = pow(weight, 2) / 2.0
            pq.push(std::make_tuple(priority, queueName));
        }
    }

    // Assign remaining slots:
    // only assign slot to a queue if the queue has active or pending transfers.
    for (int i = 0; i < maxSlots; i++) {
        if (pq.empty()) {
            break;
        }

        std::tuple<double, string> p = pq.top();
        pq.pop();
        double priority = std::get<0>(p);
        std::string queueName = std::get<1>(p);

        allocation[queueName] += 1
        activeAndPendingCounts[queueName] -= 1

        if (activeAndPendingCounts[queueName] > 0) {
            // Recompute priority and push back to pq
            double n = (double) allocation[queueName];
            priority *= (n-1) / (n+1);
            pq.push(std::make_tuple(priority, queueName));
        }
    }

    return allocation;
}

/**
 * Compute the deficit for each queue in a pair.
 * @param queueActiveCounts Number of active slots for each activity in each VO.
 * @param queueShouldBeAllocated Number of should-be-allocated slots for each activity in each VO.
*/
std::map<std::string, std::map<std::string, int>> computeDeficits(
    std::map<std::string, std::map<std::string, int>> &queueActiveCounts,
    std::map<std::string, std::map<std::string, int>> &queueShouldBeAllocated
){
    std::map<std::string, std::map<std::string, int>> result;

    for (auto i = queueActiveCounts.begin(); i != queueActiveCounts.end(); i++) {
        std::string voName = i->first;
        std::map<std::string, int> activityActiveCount = i->second;

        for (auto j = activityActiveCount.begin(); j != activityActiveCount.end(); j++) {
            std::string activityName = j->first;
            int activeCount = j->second;
            int shouldBeAllocatedCount = queueShouldBeAllocated[voName][activityName];

            result[voName][activityName] = shouldBeAllocatedCount - activeCount;
        }
    }

    return result;
}

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

} // end namespace server
} // end namespace fts3