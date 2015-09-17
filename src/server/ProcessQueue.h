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

#pragma once
#ifndef PROCESSQUEUE_H_
#define PROCESSQUEUE_H_

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/error.h"
#include "common/producer_consumer_common.h"
#include "common/Logger.h"
#include "common/ThreadSafeList.h"

#include "ws/SingleTrStateInstance.h"

extern bool stopThreads;

namespace fts3 {
namespace server {

extern time_t updateRecords;

class ProcessQueue
{
private:
    std::vector<struct message> messages;
    std::string enableOptimization;
    std::map<int, struct message_log> messagesLog;
    std::vector<struct message_updater> messagesUpdater;

public:

    /// Constructor
    ProcessQueue()
    {
        enableOptimization = config::theServerConfig().get<std::string > ("Optimizer");
        messages.reserve(600);
    }

    /// Destructor
    virtual ~ProcessQueue()
    {
    }

    /// Thread that process the queue
    void operator () ()
    {
        namespace fs = boost::filesystem;

        while (!stopThreads)
        {
            updateRecords = time(0);

            try
            {
                if (stopThreads && messages.empty() && messagesLog.empty())
                {
                    break;
                }

                //if conn to the db is lost, do not retrieve state, save it for later
                //use one fast query
                try
                {
                    db::DBSingleton::instance().getDBObjectInstance()->getDrain();
                }
                catch (...)
                {
                    sleep(10);
                    continue;
                }

                if (!fs::is_empty(fs::path(STATUS_DIR)))
                {
                    if (runConsumerStatus(messages) != 0)
                    {
                        char buffer[128] = {0};
                        FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Could not get the status messages:" << strerror_r(errno, buffer, sizeof(buffer)) << fts3::common::commit;
                    }
                }

                if (!messages.empty())
                {
                    executeUpdate(messages);

                    //now update the protocol
                    db::DBSingleton::instance().getDBObjectInstance()->updateProtocol(messages);

                    //finally clear store
                    messages.clear();
                }

                //update log file path
                if (!fs::is_empty(fs::path(LOG_DIR)))
                {
                    if (runConsumerLog(messagesLog) != 0)
                    {
                        char buffer[128] =
                        { 0 };
                        FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Could not get the log messages:" << strerror_r(errno, buffer, sizeof(buffer)) << fts3::common::commit;
                    }
                }

                try
                {
                    if (!messagesLog.empty())
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                        messagesLog.clear();
                    }
                }
                catch (std::exception& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR)<< e.what() << fts3::common::commit;

                    //try again
                    try
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                        messagesLog.clear();
                    }
                    catch(...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 1" << fts3::common::commit;
                        std::map<int, struct message_log>::const_iterator iterLogBreak;
                        for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                        {
                            struct message_log msgLogBreak = (*iterLogBreak).second;
                            runProducerLog( msgLogBreak );
                        }
                    }
                }
                catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 2" << fts3::common::commit;
                    //try again
                    try
                    {
                        db::DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                        messagesLog.clear();
                    }
                    catch(...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 3" << fts3::common::commit;
                        std::map<int, struct message_log>::const_iterator iterLogBreak;
                        for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                        {
                            struct message_log msgLogBreak = (*iterLogBreak).second;
                            runProducerLog( msgLogBreak );
                        }
                    }
                }

                //update heartbeat and progress vector
                try
                {
                    if (!fs::is_empty(fs::path(STALLED_DIR)))
                    {
                        if (runConsumerStall(messagesUpdater) != 0)
                        {
                            char buffer[128] =
                            { 0 };
                            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Could not get the updater messages:" << strerror_r(errno, buffer, sizeof(buffer)) << fts3::common::commit;
                        }
                    }

                    if(!messagesUpdater.empty())
                    {
                        std::vector<struct message_updater>::iterator iterUpdater;
                        for (iterUpdater = messagesUpdater.begin(); iterUpdater != messagesUpdater.end(); ++iterUpdater)
                        {
                            if (iterUpdater->msg_errno == 0)
                            {
                                std::string job = std::string((*iterUpdater).job_id).substr(0, 36);
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Process Updater Monitor "
                                << "\nJob id: " << job
                                << "\nFile id: " << (*iterUpdater).file_id
                                << "\nPid: " << (*iterUpdater).process_id
                                << "\nTimestamp: " << (*iterUpdater).timestamp
                                << "\nThroughput: " << (*iterUpdater).throughput
                                << "\nTransferred: " << (*iterUpdater).transferred
                                << fts3::common::commit;
                                ThreadSafeList::get_instance().updateMsg(*iterUpdater);
                            }
                        }

                        //now update the progress markers in a "bulk fashion"
                        try
                        {
                            db::DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                        }
                        catch (std::exception& e)
                        {
                            try
                            {
                                sleep(1);
                                db::DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                            }
                            catch (std::exception& e)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << fts3::common::commit;
                            }
                            catch (...)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << fts3::common::commit;
                            }
                        }
                        catch (...)
                        {
                            try
                            {
                                sleep(1);
                                db::DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                            }
                            catch (std::exception& e)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << fts3::common::commit;
                            }
                            catch (...)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << fts3::common::commit;
                            }
                        }
                    }
                    messagesUpdater.clear();
                }
                catch (const fs::filesystem_error& ex)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << fts3::common::commit;
                }
                catch (fts3::common::Err& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << fts3::common::commit;
                }
                catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << fts3::common::commit;
                }

                sleep(1);
            }
            catch (const fs::filesystem_error& ex)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<<ex.what() << fts3::common::commit;

                std::vector<struct message>::const_iterator iterBreak;
                for (iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
                {
                    struct message msgBreak = (*iterBreak);
                    runProducerStatus( msgBreak);
                }

                std::map<int, struct message_log>::const_iterator iterLogBreak;
                for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                {
                    struct message_log msgLogBreak = (*iterLogBreak).second;
                    runProducerLog( msgLogBreak );
                }

                sleep(1);
            }
            catch (std::exception& ex2)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex2.what() << fts3::common::commit;

                std::vector<struct message>::const_iterator iterBreak;
                for (iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
                {
                    struct message msgBreak = (*iterBreak);
                    runProducerStatus( msgBreak);
                }

                std::map<int, struct message_log>::const_iterator iterLogBreak;
                for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                {
                    struct message_log msgLogBreak = (*iterLogBreak).second;
                    runProducerLog( msgLogBreak );
                }

                sleep(1);
            }
            catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception" << fts3::common::commit;

                std::vector<struct message>::const_iterator iterBreak;
                for (iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
                {
                    struct message msgBreak = (*iterBreak);
                    runProducerStatus( msgBreak);
                }

                std::map<int, struct message_log>::const_iterator iterLogBreak;
                for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                {
                    struct message_log msgLogBreak = (*iterLogBreak).second;
                    runProducerLog( msgLogBreak );
                }

                sleep(1);
            }
        }
    }

private:

    void updateDatabase(const struct message& msg)
    {
        try
        {
            std::string job = std::string(msg.job_id).substr(0, 36);

            //do not process the updates here, will be done separately
            if(std::string(msg.transfer_status).compare("UPDATE") == 0)
            return;

            if (std::string(msg.transfer_status).compare("FINISHED") == 0 ||
                    std::string(msg.transfer_status).compare("FAILED") == 0 ||
                    std::string(msg.transfer_status).compare("CANCELED") == 0)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Removing job from monitoring list " << job << " " << msg.file_id << fts3::common::commit;
                ThreadSafeList::get_instance().removeFinishedTr(job, msg.file_id);
            }

            if(std::string(msg.transfer_status).compare("FAILED") == 0)
            {
                try
                {
                    //multiple replica files belonging to a job will not be retried
                    int retry = db::DBSingleton::instance().getDBObjectInstance()->getRetry(job);

                    if(msg.retry==true && retry > 0 && msg.file_id > 0 && !job.empty())
                    {
                        int retryTimes = db::DBSingleton::instance().getDBObjectInstance()->getRetryTimes(job, msg.file_id);
                        if( retryTimes <= retry-1 )
                        {
                            db::DBSingleton::instance().getDBObjectInstance()
                            ->setRetryTransfer(job, msg.file_id, retryTimes+1, msg.transfer_message);
                            return;
                        }
                    }
                }
                catch (std::exception& e)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << e.what() << fts3::common::commit;
                }
                catch (...)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << fts3::common::commit;
                }
            }

            /*session reuse process died or terminated unexpected, must terminate all files of a given job*/
            if ( (std::string(msg.transfer_message).find("Transfer terminate handler called") != std::string::npos ||
                            std::string(msg.transfer_message).find("Transfer process died") != std::string::npos ||
                            std::string(msg.transfer_message).find("because it was stalled") != std::string::npos ||
                            std::string(msg.transfer_message).find("canceled by the user") != std::string::npos ||
                            std::string(msg.transfer_message).find("undefined symbol") != std::string::npos ||
                            std::string(msg.transfer_message).find("canceled because it was not responding") != std::string::npos ))
            {
                if(std::string(msg.job_id).length() == 0)
                {
                    db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(), static_cast<int> (msg.process_id),std::string(msg.transfer_message));
                }
                else
                {
                    db::DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(msg.job_id).substr(0, 36),static_cast<int> (msg.process_id), std::string(msg.transfer_message));
                }
            }

            //update file/job state
            db::DBSingleton::instance().
            getDBObjectInstance()->
            updateTransferStatus(job, msg.file_id, msg.throughput, std::string(msg.transfer_status),
                    std::string(msg.transfer_message), static_cast<int> (msg.process_id),
                    msg.filesize, msg.timeInSecs, msg.retry);

            db::DBSingleton::instance().
            getDBObjectInstance()->
            updateJobStatus(job, std::string(msg.transfer_status), static_cast<int> (msg.process_id));

            if(std::string(msg.job_id).length() > 0 && msg.file_id > 0)
            {
                SingleTrStateInstance::instance().sendStateMessage(job, msg.file_id);
            }
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception " << e.what() << fts3::common::commit;
            struct message msgTemp = msg;
            runProducerStatus( msgTemp);
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception" << fts3::common::commit;
            struct message msgTemp = msg;
            runProducerStatus( msgTemp);
        }
    }

    void executeUpdate(std::vector<struct message>& messages)
    {
        std::vector<struct message>::const_iterator iter;
        struct message_updater msgUpdater;
        for (iter = messages.begin(); iter != messages.end(); ++iter)
        {
            try
            {
                if(stopThreads)
                {
                    std::vector<struct message>::const_iterator iterBreak;
                    for (iterBreak = messages.begin(); iterBreak != messages.end(); ++iterBreak)
                    {
                        struct message msgBreak = (*iterBreak);
                        runProducerStatus( msgBreak);
                    }

                    std::map<int, struct message_log>::const_iterator iterLogBreak;
                    for (iterLogBreak = messagesLog.begin(); iterLogBreak != messagesLog.end(); ++iterLogBreak)
                    {
                        struct message_log msgLogBreak = (*iterLogBreak).second;
                        runProducerLog( msgLogBreak );
                    }

                    break;
                }

                std::string jobId = std::string((*iter).job_id).substr(0, 36);
                strncpy(msgUpdater.job_id, jobId.c_str(), sizeof(msgUpdater.job_id));
                msgUpdater.job_id[sizeof(msgUpdater.job_id) - 1] = '\0';
                msgUpdater.file_id = (*iter).file_id;
                msgUpdater.process_id = (*iter).process_id;
                msgUpdater.timestamp = (*iter).timestamp;
                msgUpdater.throughput = 0.0;
                msgUpdater.transferred = 0.0;
                ThreadSafeList::get_instance().updateMsg(msgUpdater);

                if (iter->msg_errno == 0 && std::string((*iter).transfer_status).compare("UPDATE") != 0)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << jobId
                    << "\nFile id: " << (*iter).file_id
                    << "\nPid: " << (*iter).process_id
                    << "\nState: " << (*iter).transfer_status
                    << "\nSource: " << (*iter).source_se
                    << "\nDest: " << (*iter).dest_se << fts3::common::commit;

                    updateDatabase((*iter));
                }
            }
            catch (const boost::filesystem::filesystem_error& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << e.what() << fts3::common::commit;
            }
            catch (std::exception& ex)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << ex.what() << fts3::common::commit;
            }
            catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << fts3::common::commit;
            }
        } //end for
    }

};

} // end namespace server
} // end namespace fts3


#endif // PROCESSQUEUE_H_
