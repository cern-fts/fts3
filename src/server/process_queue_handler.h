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
#include "mq_manager.h"
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

extern bool stopThreads;


using namespace boost::interprocess;
FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

template
<
typename TRAITS
>
class ProcessQueueHandler : public TRAITS::ActiveObjectType {
protected:

    using TRAITS::ActiveObjectType::_enqueue;
    std::string enableOptimization;

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
    TRAITS::ActiveObjectType("ProcessQueueHandler", desc), qm(NULL) {
        enableOptimization = theServerConfig().get<std::string > ("Optimizer");

        try {
            qm = new QueueManager(true);
        } catch (interprocess_exception &ex) {
            /*shared mem segment already exists, reuse it*/
            try {
                if (qm)
                    delete qm;
                qm = new QueueManager(false);
            } catch (interprocess_exception &ex) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
            }
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "/dev/shm/fts3mq " << ex.what() << commit;
        }
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessQueueHandler() {
        if (qm)
            delete qm;
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
            ) {
        boost::function<void() > op = boost::bind(&ProcessQueueHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }
    
    bool updateDatabase(struct message msg){    
	       bool updated = true;    
	       std::string job = std::string(msg.job_id).substr(0, 36);
    
               if (std::string(msg.transfer_status).compare("FINISHED") == 0 && enableOptimization.compare("true") == 0) {
                    if (!(msg.nostreams == DEFAULT_NOSTREAMS && msg.buffersize == DEFAULT_BUFFSIZE && msg.timeout == DEFAULT_TIMEOUT)) {
                        updated = DBSingleton::instance().
                                getDBObjectInstance()->
                                updateOptimizer(std::string(msg.file_id), msg.filesize, msg.timeInSecs,
                                static_cast<int> (msg.nostreams), static_cast<int> (msg.timeout),
                                static_cast<int> (msg.buffersize), std::string(msg.source_se),
                                std::string(msg.dest_se));
                    }
                }
 
                /*session reuse process died or terminated unexpected*/
                if ( (updated == true) && (std::string(msg.transfer_message).find("Transfer terminate handler called") != string::npos ||
                        std::string(msg.transfer_message).find("Transfer terminate handler called") != string::npos ||
                        std::string(msg.transfer_message).find("Transfer process died") != string::npos ||
			std::string(msg.transfer_message).find("because it was stalled") != string::npos ||
			std::string(msg.transfer_message).find("canceled because it was not responding") != string::npos )) {
                    updated = DBSingleton::instance().getDBObjectInstance()->terminateReuseProcess(std::string(msg.job_id).substr(0, 36));
                }
                if(updated == true){
                updated = DBSingleton::instance().
                        getDBObjectInstance()->
                        updateFileTransferStatus(job, std::string(msg.file_id), std::string(msg.transfer_status),
                        std::string(msg.transfer_message), static_cast<int> (msg.process_id),
                        msg.filesize, msg.timeInSecs);
			
		}
                if(updated == true){		
                updated = DBSingleton::instance().
                        getDBObjectInstance()->
                        updateJobTransferStatus(std::string(msg.file_id), job, std::string(msg.transfer_status));
		}
		
		if(updated == true){		
                if (std::string(msg.transfer_status).compare("ACTIVE") != 0)
                    ThreadSafeList::get_instance().removeFinishedTr(job, atoi(std::string(msg.file_id).c_str()));
		}
		    
        return updated;		    
    }

protected:

    QueueManager* qm;
    std::vector<struct message> queueMsgRecovery;

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {

        while (stopThreads==false) { /*need to receive more than one messages at a time*/            
            try {
	    
	        bool alive = DBSingleton::instance().getDBObjectInstance()->checkConnectionStatus();
		if(!alive)
			continue;
			
                if(!queueMsgRecovery.empty()){			
			std::vector<struct message>::const_iterator iter;
			for (iter = queueMsgRecovery.begin(); iter != queueMsgRecovery.end(); ++iter) {
				bool dbUpdated = updateDatabase(*iter);
			}			
			queueMsgRecovery.clear();	
		}
	    
                struct message msg;
                bool hasMessage = qm->receive(&msg);
		if(hasMessage==false)
			continue;		
			                
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << std::string(msg.job_id).substr(0, 36)
                        << "\nFile id: " << msg.file_id
                        << "\nPid: " << msg.process_id
                        << "\nState: " << msg.transfer_status
                        << "\nMessage: " << msg.transfer_message
                        << "\nSource: " << msg.source_se
                        << "\nDest: " << msg.dest_se << commit;

		bool dbUpdated = updateDatabase(msg);
		if(!dbUpdated){			
			queueMsgRecovery.push_back(msg);
		}

            } catch (interprocess_exception &ex) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
            } catch (Err& e) {
                FTS3_COMMON_EXCEPTION_THROW(e);
            } catch (...) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Message queue thrown unhandled exception"));
            }            
        }
    }

    /* ---------------------------------------------------------------------- */
    struct TestHelper {

        TestHelper()
        : loopOver(false) {
        }

        bool loopOver;
    }
    _testHelper;
};

FTS3_SERVER_NAMESPACE_END

