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
#include "process.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include "site_name.h"
#include "FileTransferScheduler.h"

FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

template <class T>
inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

template
<
typename TRAITS
>
class ProcessServiceHandler : public TRAITS::ActiveObjectType {
protected:

    using TRAITS::ActiveObjectType::_enqueue;

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessServiceHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessServiceHandler
    (
            const std::string& desc = "" /**< Description of this service handler
            (goes to log) */
            ) :
    TRAITS::ActiveObjectType("ProcessServiceHandler", desc) {
    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessServiceHandler() {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
            ) {
        boost::function<void() > op = boost::bind(&ProcessServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
        const std::string cmd = "fts3_url_copy";
        std::string params = std::string("");
        ExecuteProcess *pr = NULL;
        std::vector<TransferJobs*> jobs2;
        std::vector<TransferJobs*>::iterator iter2;
        std::vector<TransferFiles*> files;
        std::vector<TransferFiles*>::iterator fileiter;	
	std::string sourceSiteName("");
	std::string destSiteName("");
	SiteName siteResolver;

        while(1){
        	DBSingleton::instance().getDBObjectInstance()->getSubmittedJobs(jobs2);
       		
			DBSingleton::instance().getDBObjectInstance()->getByJobId(jobs2, files);
			for (fileiter = files.begin(); fileiter != files.end(); ++fileiter) {
				TransferFiles* temp = (TransferFiles*) * fileiter;
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << temp->JOB_ID << commit;
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id: " << temp->FILE_ID << commit;
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source Url: " << temp->SOURCE_SURL << commit;
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destin Url: " << temp->DEST_SURL << commit;
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO name: " << temp->VO_NAME << commit;
				
				FileTransferScheduler scheduler(temp);
				if (scheduler.schedule()) {
					sourceSiteName = siteResolver.getSiteName(temp->SOURCE_SURL);
					destSiteName = siteResolver.getSiteName(temp->DEST_SURL);
					params.append(" -b ");
					params.append(temp->SOURCE_SURL);
					params.append(" -c ");
					params.append(temp->DEST_SURL);
					params.append(" -a ");
					params.append(temp->JOB_ID);
					params.append(" -B ");
					params.append(to_string(temp->FILE_ID));					
					params.append(" -C ");
					params.append(temp->VO_NAME);
					if(sourceSiteName.length() > 0){
						params.append(" -D ");
						params.append(sourceSiteName);
					}
					if(destSiteName.length() > 0){					
						params.append(" -E ");
						params.append(destSiteName);	
					}				

					FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << params << commit;
					pr = new ExecuteProcess(cmd, params, 0);
					if(pr){
							pr->executeProcessShell();
							delete pr;
					}
					params.clear();
				}
			}

			/** cleanup resources */
			for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
				delete *iter2;
			jobs2.clear();
			for (fileiter = files.begin(); fileiter != files.end(); ++fileiter)
				delete *fileiter;
			files.clear();
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

