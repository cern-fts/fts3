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
#include <signal.h>
#include "parse_url.h"
#include "cred-utility.h"
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>


FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;

template <class T>
inline std::string to_string(const T& t) {
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
    void killRunninfJob(std::vector<int>& requestIDs){
	std::vector<int>::iterator iter; 	
    	for (iter = requestIDs.begin(); iter != requestIDs.end(); ++iter) {
		int pid = *iter;
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Killing process: " << pid << commit;		
		kill(pid, SIGTERM);
	}
    }

    std::string extractHostname(std::string surl){
	std::string hostname("");
	char *base_scheme = NULL;
	char *base_host = NULL;
	char *base_path = NULL;
	int base_port = 0;
    	parse_url(surl.c_str(), &base_scheme, &base_host, &base_port, &base_path);
	hostname = base_host;
	if(base_scheme)
		free(base_scheme);
	if(base_host)
		free(base_host);
	if(base_path)
		free(base_path);
    	return hostname;
    }

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
	std::vector<int> requestIDs;
	std::string source_hostname("");
	std::string destin_hostname("");
	SeProtocolConfig* protocol = NULL;
	std::string proxy_file("");

        while (1) {
	    /*get jobs in submitted state*/
            DBSingleton::instance().getDBObjectInstance()->getSubmittedJobs(jobs2);
	    
	    /*also get jobs which have been canceled by the client*/
	    DBSingleton::instance().getDBObjectInstance()->getCancelJob(requestIDs);
	    if(requestIDs.size() > 0) /*if canceled jobs found and transfer already started, kill them*/
	    	killRunninfJob(requestIDs);
	    requestIDs.clear();	/*clean the list*/
	    	
            if (jobs2.size() > 0) {
	        /*get the file for each jobs*/
                DBSingleton::instance().getDBObjectInstance()->getByJobId(jobs2, files);
                for (fileiter = files.begin(); fileiter != files.end(); ++fileiter) {
                    TransferFiles* temp = (TransferFiles*) * fileiter;
		    source_hostname = extractHostname(temp->SOURCE_SURL);
		    destin_hostname = extractHostname(temp->DEST_SURL);
		    protocol = DBSingleton::instance().getDBObjectInstance()->getProtocol(source_hostname, destin_hostname);
		    proxy_file = get_proxy_cert(
                		temp->DN,                                   // user_dn
		                temp->CRED_ID,                                   // user_cred
                		temp->VO_NAME,                                   // vo_name
		                "",
                		"",                                             // assoc_service
		                "",                                             // assoc_service_type
                		false, 
                		"");

                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Job id: " << temp->JOB_ID << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "File id: " << temp->FILE_ID << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Source Url: " << temp->SOURCE_SURL << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Destin Url: " << temp->DEST_SURL << commit;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "VO name: " << temp->VO_NAME << commit;
		     
                    FileTransferScheduler scheduler(temp);
                    if (scheduler.schedule()) {
                        sourceSiteName = siteResolver.getSiteName(temp->SOURCE_SURL);
                        destSiteName = siteResolver.getSiteName(temp->DEST_SURL);
			if(proxy_file.length() > 0){
                        	params.append(" -proxy ");
	                        params.append(proxy_file);	
				/*make sure proxy is readable    */
    				chmod(proxy_file.c_str(), (mode_t) 0777); //S_IRUSR|S_IRGRP|S_IROTH				
			}		
			if(std::string(temp->CHECKSUM).length() > 0){ //checksum
                        	params.append(" -z ");
	                        params.append(temp->CHECKSUM);	
			}
			if(std::string(temp->CHECKSUM_METHOD).length() > 0){ //checksum
                        	params.append(" -A ");
	                        params.append(temp->CHECKSUM_METHOD);	
			}				
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
                        if (sourceSiteName.length() > 0) {
                            params.append(" -D ");
                            params.append(sourceSiteName);
                        }
                        if (destSiteName.length() > 0) {
                            params.append(" -E ");
                            params.append(destSiteName);
                        }
			if(std::string(temp->OVERWRITE).length() > 0){
				params.append(" -d ");
			}
			if(protocol!=NULL && protocol->NOSTREAMS > 0){
	                        params.append(" -e ");			
	                        params.append(to_string(protocol->NOSTREAMS));
			}
			if(protocol!=NULL && protocol->URLCOPY_TX_TO > 0){
	                        params.append(" -h ");			
	                        params.append(to_string(protocol->URLCOPY_TX_TO));
			}

                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << params << commit;
                        pr = new ExecuteProcess(cmd, params, 0);
                        if (pr) {
                            pr->executeProcessShell();
                            delete pr;
                        }
			DBSingleton::instance().getDBObjectInstance()->updateFileStatus(temp, "ACTIVE");
                        params.clear();
                    }
		    if(protocol)
		    	delete protocol;
                }

                /** cleanup resources */
                for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                    delete *iter2;
                jobs2.clear();
                for (fileiter = files.begin(); fileiter != files.end(); ++fileiter)
                    delete *fileiter;
                files.clear();
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

