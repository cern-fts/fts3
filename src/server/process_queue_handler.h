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
    TRAITS::ActiveObjectType("ProcessQueueHandler", desc) {
    
        enableOptimization = theServerConfig().get<std::string > ("Optimizer");
	messages.reserve(500);		
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessQueueHandler() {
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
	       
		if (std::string(msg.transfer_status).compare("FINISHED") == 0 || 
			std::string(msg.transfer_status).compare("FAILED") == 0 ||
			std::string(msg.transfer_status).compare("CANCELED") == 0){
		    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Removing job from monitoring list " << job << " " << msg.file_id << commit;
                    ThreadSafeList::get_instance().removeFinishedTr(job, msg.file_id);
		    }
			       
	       
	       int retry = DBSingleton::instance().getDBObjectInstance()->getRetry();
	       if(msg.retry==true && retry != 0 && std::string(msg.transfer_status).compare("FAILED") == 0){
 		       int retryTimes = DBSingleton::instance().getDBObjectInstance()->getRetryTimes(job, msg.file_id);
		       if(retry == -1){ //unlimited times
				DBSingleton::instance().getDBObjectInstance()->setRetryTimes(retryTimes+1, job, msg.file_id);
				DBSingleton::instance().getDBObjectInstance()->setRetryTransfer(job, msg.file_id);
				return true;		       
		       }else{	       		       
			       if(retryTimes <= retry-1 ){	
					DBSingleton::instance().getDBObjectInstance()->setRetryTimes(retryTimes+1, job, msg.file_id);
					DBSingleton::instance().getDBObjectInstance()->setRetryTransfer(job, msg.file_id);
					return true;			       					
			       }
		       }
	       }
    
               if (std::string(msg.transfer_status).compare("FINISHED") == 0 && enableOptimization.compare("true") == 0) {                   
                        updated = DBSingleton::instance().
                                getDBObjectInstance()->
                                updateOptimizer(msg.file_id, msg.filesize, msg.timeInSecs,
                                static_cast<int> (msg.nostreams), static_cast<int> (msg.timeout),
                                static_cast<int> (msg.buffersize), std::string(msg.source_se),
                                std::string(msg.dest_se));
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
                        updateFileTransferStatus(job, msg.file_id, std::string(msg.transfer_status),
                        std::string(msg.transfer_message), static_cast<int> (msg.process_id),
                        msg.filesize, msg.timeInSecs);

                	if (std::string(msg.transfer_status) == "FAILED") {
						DBSingleton::instance().
								getDBObjectInstance()->useFileReplica(msg.job_id, msg.file_id);
                	}
                }
                if(updated == true){		
                updated = DBSingleton::instance().
                        getDBObjectInstance()->
                        updateJobTransferStatus(msg.file_id, job, std::string(msg.transfer_status));
		}						                
        return updated;		    
    }

protected:

    std::vector<struct message> queueMsgRecovery;
    std::vector<struct message> messages;
    std::vector<struct message>::const_iterator iter;

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
        while (stopThreads==false) { /*need to receive more than one messages at a time*/            	    
            try {
		
	        if(fs::is_empty(fs::path(STATUS_DIR))){
			sleep(1);
			continue;
		}
	        	        
		bool alive = DBSingleton::instance().getDBObjectInstance()->checkConnectionStatus();
		if(!alive){
			sleep(1);
			continue;
		}
		
                if(!queueMsgRecovery.empty()){			
			std::vector<struct message>::const_iterator iter;
			for (iter = queueMsgRecovery.begin(); iter != queueMsgRecovery.end(); ++iter) {
				updateDatabase(*iter);
			}			
			queueMsgRecovery.clear();	
		}
	                    
        	runConsumerStatus(messages);
		if(!messages.empty()){			
		for (iter = messages.begin(); iter != messages.end(); ++iter){								
      								                
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id:" << std::string((*iter).job_id).substr(0, 36)
                        << "\nFile id: " << (*iter).file_id
                        << "\nPid: " << (*iter).process_id
                        << "\nState: " << (*iter).transfer_status
                        << "\nMessage: " << (*iter).transfer_message
                        << "\nSource: " << (*iter).source_se
                        << "\nDest: " << (*iter).dest_se << commit;
		
		/*
			exceptional case when a url-copy process fails to start because thread creation failed due to lack of resource
			do not update the database with the failed state because it will be re-scheduled
		*/
		if(std::string((*iter).transfer_message).find("thread_resource_error")!= string::npos ||
		   std::string((*iter).transfer_message).find("globus_module_activate_proxy")!= string::npos){		   	
			continue;
		}else{
			bool dbUpdated = updateDatabase((*iter));
			if(!dbUpdated){			
				queueMsgRecovery.push_back((*iter));
			}
		}
		}//end for
		messages.clear();
		}	        								    
		
		sleep(1);		
            } catch (const fs::filesystem_error& ex) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
		for (iter = messages.begin(); iter != messages.end(); ++iter)
			queueMsgRecovery.push_back(*iter);				
            } catch (Err& e) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
		for (iter = messages.begin(); iter != messages.end(); ++iter)
			queueMsgRecovery.push_back(*iter);		
            } catch (...) {
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message queue thrown unhandled exception" << commit;
		for (iter = messages.begin(); iter != messages.end(); ++iter)
			queueMsgRecovery.push_back(*iter);					
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

