/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#pragma once

#include "server_dev.h"
#include "common/pointers.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <string>
#include "SingleDbInstance.h"
#include "common/logger.h"
#include "common/error.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <boost/scoped_ptr.hpp>
#include "producer_consumer_common.h"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

extern bool stopThreads;
extern time_t updateRecords;

namespace fs = boost::filesystem;

FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

template
<
typename TRAITS
>
class ProcessQueueHandler : public TRAITS::ActiveObjectType
{
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessQueueHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessQueueHandler
    (
        const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
    ) :
        TRAITS::ActiveObjectType("ProcessQueueHandler", desc)
    {
        enableOptimization = theServerConfig().get<std::string > ("Optimizer");
        messages.reserve(600);
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessQueueHandler()
    {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
    )
    {
        boost::function<void() > op = boost::bind(&ProcessQueueHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

    void updateDatabase(const struct message& msg)
    {
        try
            {
                std::string job = std::string(msg.job_id).substr(0, 36);

                //do not process the updates here, will be done separetely
                if(std::string(msg.transfer_status).compare("UPDATE") == 0)
                    return;

                if (std::string(msg.transfer_status).compare("FINISHED") == 0 ||
                        std::string(msg.transfer_status).compare("FAILED") == 0 ||
                        std::string(msg.transfer_status).compare("CANCELED") == 0)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Removing job from monitoring list " << job << " " << msg.file_id << commit;
                        ThreadSafeList::get_instance().removeFinishedTr(job, msg.file_id);
                    }

                try
                    {
                        //multiple replica files belonging to a job will not be retried
                        int retry = DBSingleton::instance().getDBObjectInstance()->getRetry(job);
                        if(msg.retry==true && retry > 0 && std::string(msg.transfer_status).compare("FAILED") == 0 && msg.file_id > 0 && !job.empty())
                            {
                                int retryTimes = DBSingleton::instance().getDBObjectInstance()->getRetryTimes(job, msg.file_id);
                                if(retryTimes != -1 && retryTimes <= retry-1 )
                                    {
                                        DBSingleton::instance().getDBObjectInstance()
                                        ->setRetryTransfer(job, msg.file_id, retryTimes+1, msg.transfer_message);
                                        return;
                                    }
                            }
                    }
                catch (std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << e.what() << commit;
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception when set retry " << commit;
                    }

                /*session reuse process died or terminated unexpected, must terminate all files of a given job*/
                if ( (std::string(msg.transfer_message).find("Transfer terminate handler called") != string::npos ||
                        std::string(msg.transfer_message).find("Transfer process died") != string::npos ||
                        std::string(msg.transfer_message).find("because it was stalled") != string::npos ||
                        std::string(msg.transfer_message).find("canceled by the user") != string::npos ||
                        std::string(msg.transfer_message).find("undefined symbol") != string::npos ||
                        std::string(msg.transfer_message).find("canceled because it was not responding") != string::npos ))
                    {
                        if(std::string(msg.job_id).length() == 0)
                            {
                                DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(), static_cast<int> (msg.process_id),std::string(msg.transfer_message));
                            }
                        else
                            {
                                DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(msg.job_id).substr(0, 36),static_cast<int> (msg.process_id), std::string(msg.transfer_message));
                            }
                    }

                //update file/job state
                DBSingleton::instance().
                getDBObjectInstance()->
                updateFileTransferStatus(msg.throughput, job, msg.file_id, std::string(msg.transfer_status),
                                         std::string(msg.transfer_message), static_cast<int> (msg.process_id),
                                         msg.filesize, msg.timeInSecs, msg.retry);

                DBSingleton::instance().
                getDBObjectInstance()->
                updateJobTransferStatus(job, std::string(msg.transfer_status), static_cast<int> (msg.process_id));

                if(std::string(msg.job_id).length() > 0 && msg.file_id > 0)
                    {
                        SingleTrStateInstance::instance().sendStateMessage(job, msg.file_id);
                    }
            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception " << e.what() << commit;
                struct message msgTemp = msg;
                runProducerStatus( msgTemp);
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception" << commit;
                struct message msgTemp = msg;
                runProducerStatus( msgTemp);
            }
    }

protected:

    std::vector<struct message> messages;
    std::string enableOptimization;
    std::map<int, struct message_log> messagesLog;
    std::vector<struct message_updater> messagesUpdater;

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
                                                                << "\nDest: " << (*iter).dest_se << commit;

                                updateDatabase((*iter));
                            }
                    }
                catch (const fs::filesystem_error& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << e.what() << commit;
                    }
                catch (std::exception& ex)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << ex.what() << commit;
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Caught exception " << commit;
                    }
            }//end for

    }


    /* ---------------------------------------------------------------------- */
    void executeTransfer_a()
    {

        while (1)   /*need to receive more than one messages at a time*/
            {
                updateRecords = time(0);

                try
                    {
                        if(stopThreads && messages.empty() && messagesLog.empty() )
                            {
                                break;
                            }

                        //if conn to the db is lost, do not retrieve state, save it for later
                        //use one fast query
                        try
                            {
                                DBSingleton::instance().getDBObjectInstance()->getDrain();
                            }
                        catch(...)
                            {
                                sleep(10);
                                continue;
                            }

                        if(!fs::is_empty(fs::path(STATUS_DIR)))
                            {
                                if (runConsumerStatus(messages) != 0)
                                    {
                                        char buffer[128]= {0};
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the status messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                                    }
                            }

                        if(!messages.empty())
                            {

                                executeUpdate(messages);

                                /*/
                                                //first update the STATUS
                                                boost::thread_group g;

                                                std::size_t const half_size1 = messages.size() / 2;
                                                std::vector<struct message> split_1(messages.begin(), messages.begin() + half_size1);
                                                std::vector<struct message> split_2(messages.begin() + half_size1, messages.end());

                                                std::size_t const half_size2 = split_1.size() / 2;
                                                std::vector<struct message> split_11(split_1.begin(), split_1.begin() + half_size2);
                                                std::vector<struct message> split_21(split_1.begin() + half_size2, split_1.end());

                                                std::size_t const half_size3 = split_2.size() / 2;
                                                std::vector<struct message> split_12(split_2.begin(), split_2.begin() + half_size3);
                                                std::vector<struct message> split_22(split_2.begin() + half_size3, split_2.end());

                                                //create threads only when needed
                                                if(!split_11.empty())
                                                    g.create_thread(boost::bind(&ProcessQueueHandler::executeUpdate, this, boost::ref(split_11)));
                                                if(!split_21.empty())
                                                    g.create_thread(boost::bind(&ProcessQueueHandler::executeUpdate, this, boost::ref(split_21)));
                                                if(!split_12.empty())
                                                    g.create_thread(boost::bind(&ProcessQueueHandler::executeUpdate, this, boost::ref(split_12)));
                                                if(!split_22.empty())
                                                    g.create_thread(boost::bind(&ProcessQueueHandler::executeUpdate, this, boost::ref(split_22)));

                                                // wait for them
                                                g.join_all();
                                */

                                //now update the protocol
                                DBSingleton::instance().getDBObjectInstance()->updateProtocol(messages);

                                //finally clear store
                                messages.clear();
                            }

                        //update log file path
                        if (!fs::is_empty(fs::path(LOG_DIR)))
                            {
                                if(runConsumerLog(messagesLog) != 0)
                                    {
                                        char buffer[128]= {0};
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the log messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
                                    }
                            }

                        try
                            {
                                if(!messagesLog.empty())
                                    {
                                        DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                                        messagesLog.clear();
                                    }
                            }
                        catch (std::exception& e)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;

                                //try again
                                try
                                    {
                                        DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                                        messagesLog.clear();
                                    }
                                catch(...)
                                    {
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 1"  << commit;
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
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 2"  << commit;
                                //try again
                                try
                                    {
                                        DBSingleton::instance().getDBObjectInstance()->transferLogFileVector(messagesLog);
                                        messagesLog.clear();
                                    }
                                catch(...)
                                    {
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "transferLogFileVector throw exception 3" << commit;
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
                                                char buffer[128]= {0};
                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not get the updater messages:" << strerror_r(errno, buffer, sizeof(buffer)) << commit;
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
                                                                                        << commit;
                                                        ThreadSafeList::get_instance().updateMsg(*iterUpdater);
                                                    }
                                            }

                                        //now update the progress markers in a "bulk fashion"
                                        try
                                            {
                                                DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                                            }
                                        catch (std::exception& e)
                                            {
                                                try
                                                    {
                                                        sleep(1);
                                                        DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                                                    }
                                                catch (std::exception& e)
                                                    {
                                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                                                    }
                                                catch (...)
                                                    {
                                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << commit;
                                                    }
                                            }
                                        catch (...)
                                            {
                                                try
                                                    {
                                                        sleep(1);
                                                        DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(messagesUpdater);
                                                    }
                                                catch (std::exception& e)
                                                    {
                                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                                                    }
                                                catch (...)
                                                    {
                                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << commit;
                                                    }
                                            }
                                    }
                                messagesUpdater.clear();
                            }
                        catch (const fs::filesystem_error& ex)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                            }
                        catch (Err& e)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                            }
                        catch (...)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message updater thrown unhandled exception" << commit;
                            }

                        sleep(1);
                    }
                catch (const fs::filesystem_error& ex)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;

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
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex2.what() << commit;

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
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception" << commit;

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

    /* ---------------------------------------------------------------------- */
    struct TestHelper
    {

        TestHelper()
            : loopOver(false)
        {
        }

        bool loopOver;
    }
    _testHelper;
};

FTS3_SERVER_NAMESPACE_END

