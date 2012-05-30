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
    
     try {
        qm = new QueueManager(true);
    } catch (interprocess_exception &ex) {
        if (qm)
            delete qm;
	FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
    }    
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessQueueHandler() {
    	qm->remove();
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
            ) {
        boost::function<void() > op = boost::bind(&ProcessQueueHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    QueueManager* qm;

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
    
    while(1){ /*need to receive more than one messages at a time*/    
     for(int i = 0; i < 50; i++){
    	struct message* msg =  qm->receive();
          
      FTS3_COMMON_LOGGER_NEWLOG (INFO) << "MSG queue: " << msg->job_id  << commit;
      FTS3_COMMON_LOGGER_NEWLOG (INFO) << "           " << msg->file_id  << commit;      
      FTS3_COMMON_LOGGER_NEWLOG (INFO) << "           " << msg->transfer_status  << commit;            
      FTS3_COMMON_LOGGER_NEWLOG (INFO) << "           " << msg->transfer_message  << commit;   
      
      DBSingleton::instance().getDBObjectInstance()->updateFileTransferStatus(std::string(msg->job_id), std::string(msg->file_id),std::string(msg->transfer_status),std::string(msg->transfer_message) );
      DBSingleton::instance().getDBObjectInstance()->updateJobTransferStatus(std::string(msg->file_id), std::string(msg->job_id), std::string(msg->transfer_status));      
      
      if(msg)
      	delete msg;                
      }
      sleep(1);
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

