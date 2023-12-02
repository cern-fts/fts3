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


    schedulerFunction = Scheduler::getSchedulerFunction();

    allocatorFunction = Allocator::getAllocatorFunction();
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


void TransfersService::getFiles(const std::vector<QueueId>& queues, int availableUrlCopySlots)
{
    auto db = DBSingleton::instance().getDBObjectInstance();

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);
    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    for (auto i = queues.begin(); i != queues.end(); ++i) {
        // To reduce queries, fill in one go limits as source and as destination
        if (slotsLeftForDestination.count(i->destSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->destSe);
            slotsLeftForDestination[i->destSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->destSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        if (slotsLeftForSource.count(i->sourceSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->sourceSe);
            slotsLeftForDestination[i->sourceSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->sourceSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        // Once it is filled, decrement
        slotsLeftForDestination[i->destSe] -= i->activeCount;
        slotsLeftForSource[i->sourceSe] -= i->activeCount;
    }

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
 * Execute the file transfer.
 * (This was originally in getFiles.)
 * This is also where we consider the slots left for dest and src nodes.
 * TODO: The Allocator already takes into consideration slotsLeftForDestination and slotsLeftForSource.
 *       The code here will be modified in the future to remove redundancy.
 *       For now this function will keep the same code as the original getFiles() to ensure correctness.
*/
void TransfersService::executeFileTransfers(
    std::map<std::string, std::list<TransferFile>> scheduledFiles, 
    int availableUrlCopySlots,
    std::vector<QueueId> queues
){
    auto db = DBSingleton::instance().getDBObjectInstance();

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);
    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    for (auto i = queues.begin(); i != queues.end(); ++i) {
        // To reduce queries, fill in one go limits as source and as destination
        if (slotsLeftForDestination.count(i->destSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->destSe);
            slotsLeftForDestination[i->destSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->destSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        if (slotsLeftForSource.count(i->sourceSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->sourceSe);
            slotsLeftForDestination[i->sourceSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->sourceSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        // Once it is filled, decrement
        slotsLeftForDestination[i->destSe] -= i->activeCount;
        slotsLeftForSource[i->sourceSe] -= i->activeCount;
    }

    try 
    {
        if (scheduledFiles.empty())
            return;

        // Count of scheduled transfers for activity
        std::map<std::string, int> scheduledByActivity;

        // create transfer-file handler
        TransferFileHandler tfh(scheduledFiles);

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
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TransfersService:scheduleFiles" << commit;
        execPool.interrupt();
        execPool.join();
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService:scheduleFiles " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService!" << commit;
    }
}


void TransfersService::executeUrlcopy()
{
    std::vector<QueueId> queues;
    boost::thread_group g;

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
        DBSingleton::instance().getDBObjectInstance()->getQueuesWithPending(queues);
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
        time_t end = time(0); //std::chrono::system_clock::now();
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                        << "func=\"executeUrlcopy\" "
                                        << "DBcall=\"getQueuesWithPending\" " 
                                        << "time=\"" << end - start << "\"" 
                                        << commit;
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
