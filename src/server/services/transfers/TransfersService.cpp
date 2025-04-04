/*
 * Copyright (c) CERN 2013-2025
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

#include <ctime>
#include <random>

#include "TransfersService.h"
#include "VoShares.h"

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/ThreadPool.h"

#include "cred/DelegCred.h"

#include "db/generic/TransferFile.h"

#include "server/common/DrainMode.h"

#include "TransferFileHandler.h"
#include "FileTransferExecutor.h"

#include "msg-bus/producer.h"


using namespace fts3::common;


namespace fts3 {
namespace server {


static void postgresStoreMaxUrlCopyProcessesInDb() {
    try {
        auto db = DBSingleton::instance().getDBObjectInstance();
        if (!db) {
            throw std::runtime_error("Failed to get a pointer the DBSingleton object");
        }

        const int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
        db->postgresStoreMaxUrlCopyProcesses(maxUrlCopy);
    } catch(std::exception &se) {
        std::ostringstream msg;
        msg << __FUNCTION__ << ": Failed: " << se.what();
        throw std::runtime_error(msg.str());
    } catch(...) {
        std::ostringstream msg;
        msg << __FUNCTION__ << ": Failed: Caught an unknown exception";
        throw std::runtime_error(msg.str());
    }
}


TransfersService::TransfersService(): BaseService("TransfersService")
{
    cmd = "fts_url_copy";

    logDir = config::ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = config::ServerConfig::instance().get<std::string>("Alias");

    monitoringMessages = config::ServerConfig::instance().get<bool>("MonitoringMessaging");
    schedulingInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("SchedulingInterval");
}


void TransfersService::runService()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TransfersService interval: " << schedulingInterval.total_seconds() << "s" << commit;

    while (!boost::this_thread::interruption_requested())
    {
        updateLastRunTimepoint();

        if (config::ServerConfig::instance().get<std::string>("DbType") == "postgresql") {
            postgresStoreMaxUrlCopyProcessesInDb();
        }

        try
        {
            boost::this_thread::sleep(schedulingInterval);

            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            if ("mysql" == DBSingleton::instance().getDBObjectInstance()->getDbtype()) {
                executeUrlCopy();
            } else {
                postgresExecuteUrlCopy();
            }
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TransfersService!" << commit;
            break;
        } catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TransfersService!" << commit;
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

        // Count available url-copy slots right before start to fork new url-copy processes
        int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
        int urlCopyCount = countProcessesWithName("fts_url_copy");
        availableUrlCopySlots = maxUrlCopy - urlCopyCount;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Number of fts_url_copy process: " << urlCopyCount << commit;

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
                        monitoringMessages, ftsHostName,
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
    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Interruption requested in TransfersService::getFiles!" << commit;
        execPool.interrupt();
        execPool.join();
        throw;
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService::getFiles: " << e.what() << commit;
        throw;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TransfersService::getFiles!" << commit;
        throw;
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


void TransfersService::executeUrlCopy()
{
    std::vector<QueueId> queues, unschedulable;

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
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(queues.begin(), queues.end(), g);
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
    } catch (const boost::thread_interrupted&) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Interruption requested in TransfersService::executeUrlCopy!" << commit;
        throw;
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TransfersService::executeUrlCopy: " << e.what() << commit;
        throw;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TransfersService::executeUrlCopy!" << commit;
        throw;
    }
}


void TransfersService::postgresExecuteUrlCopy() {
    const int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
    const int urlCopyCount = countProcessesWithName("fts_url_copy");
    const int availableUrlCopySlots = maxUrlCopy - urlCopyCount;

    // Bail out as soon as possible if there are too many url-copy processes
    if (availableUrlCopySlots <= 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING)
            << "Reached limitation of MaxUrlCopyProcesses"
            << commit;
        return;
    }

    const time_t start = time(0);
    std::list<TransferFile> scheduledFiles =
        DBSingleton::instance().getDBObjectInstance()->postgresGetScheduledFileTransfers(
            availableUrlCopySlots
        );
    const time_t elapsed = time(0) - start;
    if (scheduledFiles.empty()) {
        return;
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TransfersService\" "
                                    << "func=\"postgresqlExecuteUrlcopy\" "
                                    << "DBcall=\"getScheduledFileTransfers\" "
                                    << "time=\"" << elapsed << "\" "
                                    << "nbScheduledFiles=\"" << scheduledFiles.size() << "\""
                                    << commit;

    ThreadPool<FileTransferExecutor> execPool(execPoolSize);
    std::map<std::pair<std::string, std::string>, std::string> proxies;

    for (TransferFile &scheduledFile: scheduledFiles) {
        const std::pair<std::string, std::string> proxy_key(
            scheduledFile.credId,
            scheduledFile.userDn
        );

        if (proxies.find(proxy_key) == proxies.end())
        {
            proxies[proxy_key] = DelegCred::getProxyFile(
                scheduledFile.userDn,
                scheduledFile.credId
            );
        }

        FileTransferExecutor * const exec = new FileTransferExecutor(
            scheduledFile,
            monitoringMessages,
            ftsHostName,
            proxies[proxy_key],
            logDir,
            msgDir
        );
        execPool.start(exec);
    }
    execPool.join();
}


} // end namespace server
} // end namespace fts3
