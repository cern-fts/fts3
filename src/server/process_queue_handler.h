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
        
        messages.reserve(3000);
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

    static bool updateDatabase(const struct message& msg)
    {
        bool updated = true;
	static std::string enableOptimization = theServerConfig().get<std::string > ("Optimizer");
        try
            {
                std::string job = std::string(msg.job_id).substr(0, 36);

                if(std::string(msg.transfer_status).compare("UPDATE") == 0)
                    {
                        DBSingleton::instance().getDBObjectInstance()->updateProtocol(job, msg.file_id,
                                static_cast<int> (msg.nostreams),
                                static_cast<int> (msg.timeout),
                                static_cast<int> (msg.buffersize),
                                msg.filesize);
                        return true;
                    }

                if (std::string(msg.transfer_status).compare("FINISHED") == 0 ||
                        std::string(msg.transfer_status).compare("FAILED") == 0 ||
                        std::string(msg.transfer_status).compare("CANCELED") == 0)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Removing job from monitoring list " << job << " " << msg.file_id << commit;
                        ThreadSafeList::get_instance().removeFinishedTr(job, msg.file_id);
                    }

                int retry = DBSingleton::instance().getDBObjectInstance()->getRetry(job);
                if(msg.retry==true && retry > 0 && std::string(msg.transfer_status).compare("FAILED") == 0)
                    {
                        int retryTimes = DBSingleton::instance().getDBObjectInstance()->getRetryTimes(job, msg.file_id);
                        if(retry == -1)  //unlimited times
                            {
                                DBSingleton::instance().getDBObjectInstance()
                                ->setRetryTransfer(job, msg.file_id, retryTimes+1, msg.transfer_message);
                                SingleTrStateInstance::instance().sendStateMessage(job, msg.file_id);
                                return true;
                            }
                        else
                            {
                                if(retryTimes <= retry-1 )
                                    {
                                        DBSingleton::instance().getDBObjectInstance()
                                        ->setRetryTransfer(job, msg.file_id, retryTimes+1, msg.transfer_message);
                                        SingleTrStateInstance::instance().sendStateMessage(job, msg.file_id);
                                        return true;
                                    }
                            }
                    }

                if (std::string(msg.transfer_status).compare("FINISHED") == 0 && enableOptimization.compare("true") == 0)
                    {
                        updated = DBSingleton::instance().
                                  getDBObjectInstance()->
                                  updateOptimizer(msg.throughput, msg.file_id, msg.filesize, msg.timeInSecs,
                                                  static_cast<int> (msg.nostreams), static_cast<int> (msg.timeout),
                                                  static_cast<int> (msg.buffersize), std::string(msg.source_se),
                                                  std::string(msg.dest_se));
                    }

                /*session reuse process died or terminated unexpected*/
                if ( (updated == true) && (std::string(msg.transfer_message).find("Transfer terminate handler called") != string::npos ||
                                           std::string(msg.transfer_message).find("Transfer terminate handler called") != string::npos ||
                                           std::string(msg.transfer_message).find("Transfer process died") != string::npos ||
                                           std::string(msg.transfer_message).find("because it was stalled") != string::npos ||
                                           std::string(msg.transfer_message).find("canceled because it was not responding") != string::npos ))
                    {
                        updated = DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(msg.job_id).substr(0, 36));
                    }
                if(updated == true)
                    {
                        updated = DBSingleton::instance().
                                  getDBObjectInstance()->
                                  updateFileTransferStatus(msg.throughput, job, msg.file_id, std::string(msg.transfer_status),
                                                           std::string(msg.transfer_message), static_cast<int> (msg.process_id),
                                                           msg.filesize, msg.timeInSecs);

                        if (std::string(msg.transfer_status) == "FAILED")
                            {
                                DBSingleton::instance().
                                getDBObjectInstance()->useFileReplica(msg.job_id, msg.file_id);
                            }
                    }
                if(updated == true)
                    {
                        updated = DBSingleton::instance().
                                  getDBObjectInstance()->
                                  updateJobTransferStatus(msg.file_id, job, std::string(msg.transfer_status));
                    }

                SingleTrStateInstance::instance().sendStateMessage(job, msg.file_id);
            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception " << e.what() << commit;
                throw;
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue updateDatabase throw exception" << commit;
                throw;
            }
        return updated;
    }

protected:

    std::vector<struct message> queueMsgRecovery;
    std::vector<struct message> messages;
    std::vector<struct message>::const_iterator iter;
    struct message_updater msgUpdater;

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a()
    {
        while (1)   /*need to receive more than one messages at a time*/
            {
                try
                    {
                        if(stopThreads && messages.empty() && queueMsgRecovery.empty() )
                            {
                                break;
                            }


                        if(fs::is_empty(fs::path(STATUS_DIR)))
                            {
                        	usleep(300000);
                                continue;
                            }

                        if(!queueMsgRecovery.empty())
                            {
                                std::vector<struct message>::const_iterator iter;
                                for (iter = queueMsgRecovery.begin(); iter != queueMsgRecovery.end(); ++iter)
                                    {
                                        updateDatabase(*iter);
                                    }
                                queueMsgRecovery.clear();
                            }

                        if (runConsumerStatus(messages) != 0)
                            {
                                char buffer[128]= {0};
                                throw Err_System(std::string("Could not get the status messages: ") +
                                                 strerror_r(errno, buffer, sizeof(buffer)));
                            }

                        if(!messages.empty())
                            {
			    
                                for (iter = messages.begin(); iter != messages.end(); ++iter)
                                    {					
					std::string jobId = std::string((*iter).job_id).substr(0, 36);
    					strcpy(msgUpdater.job_id, jobId.c_str());
    					msgUpdater.file_id = (*iter).file_id;
    					msgUpdater.process_id = (*iter).process_id;
    					msgUpdater.timestamp = (*iter).timestamp;
    					msgUpdater.throughput = 0.0;
    					msgUpdater.transferred = 0.0;					
					ThreadSafeList::get_instance().updateMsg(msgUpdater);					
					
                                        if (iter->msg_errno == 0)
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << jobId
                                                                                << "\nFile id: " << (*iter).file_id
                                                                                << "\nPid: " << (*iter).process_id
                                                                                << "\nState: " << (*iter).transfer_status                                                                                
                                                                                << "\nSource: " << (*iter).source_se
                                                                                << "\nDest: " << (*iter).dest_se << commit;

                                                /*
                                                    exceptional case when a url-copy process fails to start because thread creation failed due to lack of resource
                                                    do not update the database with the failed state because it will be re-scheduled
                                                */
                                                if(std::string((*iter).transfer_message).find("thread_resource_error")!= string::npos ||
                                                        std::string((*iter).transfer_message).find("globus_module_activate_proxy")!= string::npos)
                                                    {
                                                        continue;
                                                    }
                                                else
                                                    {
                                                        bool dbUpdated = updateDatabase((*iter));
                                                        if(!dbUpdated)
                                                            {
                                                                queueMsgRecovery.push_back((*iter));
                                                            }
                                                    }
                                            }
                                        else
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to read a status message: "
                                                                               << iter->msg_error_reason << commit;
                                            }
                                    }//end for
                                messages.clear();
                            }

                        usleep(300000);
                    }
                catch (const fs::filesystem_error& ex)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                        for (iter = messages.begin(); iter != messages.end(); ++iter)
                            queueMsgRecovery.push_back(*iter);
                        usleep(300000);
                    }
                catch (Err& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
                        for (iter = messages.begin(); iter != messages.end(); ++iter)
                            queueMsgRecovery.push_back(*iter);
                        usleep(300000);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception" << commit;
                        for (iter = messages.begin(); iter != messages.end(); ++iter)
                            queueMsgRecovery.push_back(*iter);
                        usleep(300000);
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

