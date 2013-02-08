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
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include "config/serverconfig.h"
#include "definitions.h"
#include "queue_updater.h"
#include <boost/algorithm/string.hpp>  
#include "mq_manager.h"
#include <boost/interprocess/ipc/message_queue.hpp>

extern bool stopThreads;


FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;
using namespace boost::interprocess;

template
<
typename TRAITS
>
class ProcessUpdaterDBServiceHandler : public TRAITS::ActiveObjectType {
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessUpdaterDBServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessUpdaterDBServiceHandler
    (
            const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
            ) :
    TRAITS::ActiveObjectType("ProcessUpdaterDBServiceHandler", desc) {
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessUpdaterDBServiceHandler() {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
            ) {

        boost::function<void() > op = boost::bind(&ProcessUpdaterDBServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
        while (stopThreads==false) { /*need to receive more than one messages at a time*/
	 try{
            bool alive = DBSingleton::instance().getDBObjectInstance()->checkConnectionStatus();
	    if(!alive){
	        sleep(10);
		continue;		    
	    }
	    
            std::vector<struct message_updater> messages;	    
            ThreadSafeList::get_instance().checkExpiredMsg(messages);	    

            if (!messages.empty()) {
                bool updated = DBSingleton::instance().getDBObjectInstance()->retryFromDead(messages);
		if(updated){
                	ThreadSafeList::get_instance().deleteMsg(messages);
                	messages.clear();
		}
            }
	    
	    
	    /*set to fail all old queued jobs which have exceeded max queue time*/
	    DBSingleton::instance().getDBObjectInstance()->setToFailOldQueuedJobs();
            sleep(10);
        }catch (...) {
	        sleep(10);	
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Message updater thrown unhandled exception"));
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

