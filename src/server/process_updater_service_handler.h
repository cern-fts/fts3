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
#include "producer_consumer_common.h"
#include <boost/filesystem.hpp>

extern bool stopThreads;

namespace fs = boost::filesystem;

FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

template <class T>
inline std::string to_stringT(const T& t) {
    std::stringstream ss;
    ss << t;
    return ss.str();
}

template
<
typename TRAITS
>
class ProcessUpdaterServiceHandler : public TRAITS::ActiveObjectType {
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessUpdaterServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessUpdaterServiceHandler
    (
            const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
            ) :
    TRAITS::ActiveObjectType("ProcessUpdaterServiceHandler", desc) { 
    
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessUpdaterServiceHandler() {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
            ) {

        boost::function<void() > op = boost::bind(&ProcessUpdaterServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
        
        std::vector<struct message_updater> messages;
	std::vector<struct message_updater>::const_iterator iter;
    
        while (stopThreads==false) { /*need to receive more than one messages at a time*/
            try {
	    
	      if(fs::is_empty(fs::path(STALLED_DIR))){
			sleep(10);
			continue;
		}
	    
	        runConsumerStall(messages);
		if(messages.empty()){
			sleep(10);		
			continue;
		}else{
			for (iter = messages.begin(); iter != messages.end(); ++iter){
				std::string job = std::string((*iter).job_id).substr(0, 36);
                		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Process Updater Monitor "
                        		<< "\nJob id: " << job
                        		<< "\nFile id: " << (*iter).file_id
                        		<< "\nPid: " << (*iter).process_id
                        		<< "\nTimestamp: " << (*iter).timestamp << commit;	
                		ThreadSafeList::get_instance().updateMsg(*iter);
			}						
			messages.clear();
		}
	      sleep(10);
            } catch (const fs::filesystem_error& ex) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom(ex.what()));
	      sleep(10);		
            } catch (Err& e) {
                FTS3_COMMON_EXCEPTION_THROW(e);
	      sleep(10);		
            } catch (...) {
                FTS3_COMMON_EXCEPTION_THROW(Err_Custom("Message queue threw unhandled exception"));
	      sleep(10);		
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

