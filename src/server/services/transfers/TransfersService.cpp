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

#include "TransfersService.h"
#include "VoShares.h"

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/ThreadPool.h"

#include "cred/DelegCred.h"

#include "db/generic/TransferFile.h"

#include "server/DrainMode.h"

#include "TransferFileHandler.h"
#include "FileTransferExecutor.h"

#include <msg-bus/producer.h>
#include "MaximumFlow.h"

#include <ctime>

using namespace fts3::common;


namespace fts3 {
namespace server {

extern time_t retrieveRecords;


TransfersService::TransfersService(): BaseService("TransfersService")
{
    cmd = "fts_url_copy";

    logDir = config::ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = config::ServerConfig::instance().get<std::string>("Alias");
    infosys = config::ServerConfig::instance().get<std::string>("Infosys");

    monitoringMessages = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
    schedulingInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("SchedulingInterval");
    dummyLambda = 0; 
}


TransfersService::~TransfersService()
{
}


void TransfersService::runService()
{
    while (!boost::this_thread::interruption_requested())
    {
        retrieveRecords = time(0);

        try
        {
            boost::this_thread::sleep(schedulingInterval);

            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            executeUrlcopy();
        }
        catch (boost::thread_interrupted&)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested" << commit;
            break;
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService " << e.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
        }
    }
}

void getSourceDestinationCapacities(std::map<std::string, int>& slotsLeftForSource, std::map<std::string, int>& slotsLeftForDestination, const std::vector<QueueId>& queues) {
    auto db = DBSingleton::instance().getDBObjectInstance();
    for (const auto& i : queues) {
        // To reduce queries, fill in one go limits as source and as destination
        if (slotsLeftForDestination.count(i.destSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i.destSe);
            slotsLeftForDestination[i.destSe] = seConfig.inboundMaxActive > 0 ? seConfig.inboundMaxActive : 60;
            slotsLeftForSource[i.destSe] = seConfig.outboundMaxActive > 0 ? seConfig.outboundMaxActive : 60;
        }
        if (slotsLeftForSource.count(i.sourceSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i.sourceSe);
            slotsLeftForDestination[i.sourceSe] = seConfig.inboundMaxActive > 0 ? seConfig.inboundMaxActive : 60;
            slotsLeftForSource[i.sourceSe] = seConfig.outboundMaxActive > 0 ? seConfig.outboundMaxActive : 60;
        }
        // Once it is filled, decrement
        slotsLeftForDestination[i.destSe] -= i.activeCount;
        slotsLeftForSource[i.sourceSe] -= i.activeCount;
    }
}

void TransfersService::getFiles(const std::vector<QueueId>& queues, int availableUrlCopySlots)
{
    auto db = DBSingleton::instance().getDBObjectInstance();

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);
    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    getSourceDestinationCapacities(slotsLeftForSource, slotsLeftForDestination, queues);
    try
    {
        if (queues.empty())
            return;

        // now get files to be scheduled
        std::map<std::string, std::list<TransferFile> > voQueues;


        time_t start = time(0);
        db->getReadyTransfers(queues, voQueues);
        time_t end =time(0);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                        << "func=\"getFiles\" "
                                        << "DBcall=\"getReadyTransfers\" " 
                                        << "time=\"" << end - start << "\"" 
                                        << commit;

        if (voQueues.empty())
            return;

        // Count of scheduled transfers for activity
        std::map<std::string, int> scheduledByActivity;

        // create transfer-file handler
        TransferFileHandler tfh(voQueues);

        std::map<std::pair<std::string, std::string>, std::string> proxies;

        // loop until all files have been served
        int initial_size = tfh.size();

        std::set<std::string> warningPrintedSrc, warningPrintedDst;
        while (!tfh.empty() && availableUrlCopySlots > 0)
        {
            // iterate over all VOs
            for (auto it_vo = tfh.begin(); it_vo != tfh.end() && availableUrlCopySlots > 0; it_vo++)
            {
                if (boost::this_thread::interruption_requested())
                {
                    execPool.interrupt();
                    return;
                }

                boost::optional<TransferFile> opt_tf = tfh.get(*it_vo);
                // if this VO has no more files to process just continue
                if (!opt_tf)
                    continue;

                TransferFile & tf = *opt_tf;

                // just to be sure
                if (tf.fileId == 0 || tf.userDn.empty() || tf.credId.empty())
                    continue;

                if (!scheduledByActivity.count(tf.activity)) {
                    scheduledByActivity[tf.activity] = 0;
                }

                std::pair<std::string, std::string> proxy_key(tf.credId, tf.userDn);

                if (proxies.find(proxy_key) == proxies.end())
                {
                    proxies[proxy_key] = DelegCred::getProxyFile(tf.userDn, tf.credId);
                }

                if (slotsLeftForDestination[tf.destSe] <= 0) {
                    if (warningPrintedDst.count(tf.destSe) == 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                            << "Reached limitation for destination " << tf.destSe
                            << commit;
                        warningPrintedDst.insert(tf.destSe);
                    }
                }
                else if (slotsLeftForSource[tf.sourceSe] <= 0) {
                    if (warningPrintedSrc.count(tf.sourceSe) == 0) {
                        FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                            << "Reached limitation for source " << tf.sourceSe
                            << commit;
                        warningPrintedSrc.insert(tf.sourceSe);
                    }
                } else {
                    // Increment scheduled transfers by activity
                    scheduledByActivity[tf.activity]++;

                    FileTransferExecutor *exec = new FileTransferExecutor(tf,
                        monitoringMessages, infosys, ftsHostName,
                        proxies[proxy_key], logDir, msgDir);

                    execPool.start(exec);
                    --availableUrlCopySlots;
                    --slotsLeftForDestination[tf.destSe];
                    --slotsLeftForSource[tf.sourceSe];
                }
            }
        }

        if (availableUrlCopySlots <= 0 && !tfh.empty()) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                << "Reached limitation of MaxUrlCopyProcesses"
                << commit;
        }

        // wait for all the workers to finish
        execPool.join();
        int scheduled = execPool.reduce(std::plus<int>());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) <<"Threadpool processed: " << initial_size
                << " files (" << scheduled << " have been scheduled)" << commit;

        if (scheduled > 0) {
            std::ostringstream out;

            for (auto it_activity = scheduledByActivity.begin();
                 it_activity != scheduledByActivity.end(); ++it_activity) {
                std::string activity_name = it_activity->first;
                std::replace(activity_name.begin(), activity_name.end(), ' ', '_');
                out << " " << activity_name << "=" << it_activity->second;
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Scheduled by activity:" << out.str() << commit;
        }
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService:getFiles" << commit;
        execPool.interrupt();
        execPool.join();
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService:getFiles " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
    }
}


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

enum AllocatorAlgorithm {
    MAXIMUM_FLOW,
    GREEDY,
    INVALID_ALLOCATOR_ALGORITHM,
};

AllocatorAlgorithm getAllocatorAlgorithm() {
    std::string allocatorConfig = config::ServerConfig::instance().get<std::string>("TransferServiceAllocatorAlgorithm");
    if (allocatorConfig == "MAXIMUM_FLOW") {
        return MAXIMUM_FLOW;
    }
    else if(allocatorConfig == "GREEDY") {
        return GREEDY;
    }
    else {
        return INVALID_ALLOCATOR_ALGORITHM;
    }
}

void TransfersService::executeUrlcopy()
{
    std::vector<QueueId> queues, unschedulable;
    boost::thread_group g;
    auto db = DBSingleton::instance().getDBObjectInstance();
    // Bail out as soon as possible if there are too many url-copy processes
    int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
    int urlCopyCount = countProcessesWithName("fts_url_copy");
    int availableUrlCopySlots = maxUrlCopy - urlCopyCount;

    if (availableUrlCopySlots <= 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING)
            << "Reached limitation of MaxUrlCopyProcesses"
            << commit;
        return;
    }

    try {
        time_t start = time(0); //std::chrono::system_clock::now();
        time_t end;
        // Retrieve queues
        DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
        std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
        getSourceDestinationCapacities(slotsLeftForSource, slotsLeftForDestination, queues);
        // Retrieve link capacities
        std::map<std::string, std::list<TransferFile> > voQueues;
        std::map<std::pair<std::string, std::string>, int> linkCapacities = db->getLinkCapacities(queues, voQueues);
        std::map<std::string, int> seToInt;
        std::map<int, std::string> intToSe;
        int nodeCount;
        MaximumFlow::Dinics solver;
        std::map<std::pair<std::string, std::string>, int> allocatorMap;
        std::map<std::pair<int, int>, int> maximumFlow;
        std::map<std::pair<int, int>, int> starvedLinks; 
        switch(getAllocatorAlgorithm()) {
            case (MAXIMUM_FLOW):
                nodeCount = 0;
                // We initialize seToInt and intToSe since our maximum flow class utilizes array indexing for optimization
                for (const auto& i : queues) {
                    if (seToInt.find(i.sourceSe) == seToInt.end()) {
                        seToInt[i.sourceSe] = nodeCount;
                        intToSe[nodeCount] = i.sourceSe;
                        nodeCount++;
                    }
                    if (seToInt.find(i.destSe) == seToInt.end()) {
                        seToInt[i.destSe] = nodeCount;
                        intToSe[nodeCount] = i.destSe;
                        nodeCount++;
                    }
                    // determine if the link is starved
                    std::pair<int, int> currLink = std::make_pair(seToInt[i.sourceSe], seToInt[i.destSe]); 
                    if (linkDeficitTracker[currLink] >= dummyLamda) {
                        // keep track of which links are starved, and allocate maximum capacity (at first, to be updated below)
                        starvedLinks[currLink] = linkCapacities[currLink]; 
                    }
                }
                // create a vector of starved links from the map
                std::vector<std::pair<std::pair<int, int>, int>> sortedStarvedLinks(starvedLinks.begin(), starvedLinks.end()); 
                auto compareLinks = [](const std::pair<std::pair<int, int>, int>& a, std::pair<std::pair<int, int>, int>& b) {
                    return a.second > b.second; 
                }
                // sort the starved links in decreasing order 
                std::sort(sortedStarvedLinks.begin(), sortedStarvedLinks.end(), compareLinks); 
                // allocate slots for the starved links 
                for (const auto& elem : sortedStarvedLinks) {
                    // allocate slots to given link 
                    int minCapacityStorageElements = min(slotsLeftForSource[elem.first.first], slotsLeftForDestination[elem.first.second]);
                    int allocated = min(minCapacityStorageElements, elem.second);
                    allocatorMap[elem.first] = allocated; 
                    // update storage elements' slots 
                    slotsLeftForSource[elem.first.first] -= allocated;
                    slotsLeftForDestination[elem.first.second] -= allocated; 
                    // update deficit tracker 
                    linkDeficitTracker[elem.first] = linkCapacities[elem.first] - allocated; 
                }
                // node count is incremented after each node so virtual source is nodeCount, dest is nodeCount + 1
                solver.setSource(nodeCount);
                solver.setSink(nodeCount + 1);
                solver.setNodes(nodeCount + 2);
                // Introduce capacities for virtual source to each source
                for (const auto& i : queues) {
                    // if the link isn't starved, add it to the graph
                    if (starvedLinks.find(std::make_pair(i.sourceSe, i.destSe)) == starvedLinks.end()) {
                        solver.addEdge(nodeCount, seToInt[i.sourceSe], slotsLeftForSource[i.sourceSe]);
                        solver.addEdge(seToInt[i.destSe], nodeCount + 1, slotsLeftForDestination[i.destSe]);
                        solver.addEdge(seToInt[i.sourceSe], seToInt[i.destSe], linkCapacities[std::make_pair(i.sourceSe, i.destSe)]);
                    }
                }
                
                maximumFlow = solver.computeMaximumFlow();
                
                for (const auto& entry : maximumFlow) {
                    allocatorMap[std::make_pair(intToSe[entry.first.first], intToSe[entry.first.second])] = entry.second;
                }
                break;
                
            case (GREEDY):
                // Breaking determinism. See FTS-704 for an explanation.
                std::random_shuffle(queues.begin(), queues.end());
                // Apply VO shares at this level. Basically, if more than one VO is used the same link,
                // pick one each time according to their respective weights
                queues = applyVoShares(queues, unschedulable);
                // Fail all that are unschedulable
                failUnschedulable(unschedulable);

                if (queues.empty()) {
                    return;
                }
                getFiles(queues, availableUrlCopySlots);
                end = time(0); //std::chrono::system_clock::now();
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                                << "func=\"executeUrlcopy\" "
                                                << "DBcall=\"getQueuesWithPending\" " 
                                                << "time=\"" << end - start << "\"" 
                                                << commit;
                break;
            default:
                throw std::runtime_error("Invalid Allocator Algorithm");
        }
        
    }
    catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService" << commit;
        g.interrupt_all();
        g.join_all();
        throw;
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService " << e.what() << commit;
        throw;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
        throw;
    }
}

} // end namespace server
} // end namespace fts3
