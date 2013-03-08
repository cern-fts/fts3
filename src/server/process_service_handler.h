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
#include <map>
#include <list>
#include <string>
#include <vector>
#include <sstream>
#include "site_name.h"
#include "FileTransferScheduler.h"
#include "ConfigurationAssigner.h"
#include "ProtocolResolver.h"
#include <signal.h>
#include "parse_url.h"
#include "cred-utility.h"
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fstream>
#include "config/serverconfig.h"
#include "definitions.h"
#include "DrainMode.h"
#include "StaticSslLocking.h"
#include "queue_updater.h"
#include <boost/algorithm/string.hpp>  
#include <sys/param.h>
#include <boost/scoped_ptr.hpp>
#include "name_to_uid.h"
#include "producer_consumer_common.h"
#include "TransferFileHandler.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>    

extern bool stopThreads;


FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

/*resource resource management as not to run out of memory or too many processes*/
static int getMaxThreads() {
    struct rlimit rlim;
    int err = -1;
    err = getrlimit(RLIMIT_NPROC, &rlim);
    if (0 != err) {
        return -1;
    } else {
        return rlim.rlim_cur;
    }
    return -1;
}

static int getAvailableMemory() {
    const double megabyte = 1024 * 1024;
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return -1;

    return (info.freeram) / megabyte;
}

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
    std::string enableOptimization;

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
        maximumThreads = getMaxThreads();        

        enableOptimization = theServerConfig().get<std::string > ("Optimizer");
        char hostname[MAXHOSTNAMELEN];
        gethostname(hostname, MAXHOSTNAMELEN);
        ftsHostName = std::string(hostname);
        allowedVOs = std::string("");
	infosys = theServerConfig().get<std::string > ("Infosys");	
        const vector<std::string> voNameList(theServerConfig().get< vector<string> >("AuthorizedVO"));
        if (voNameList.size() > 0 && std::string(voNameList[0]).compare("*") != 0) {
            std::vector<std::string>::const_iterator iterVO;
            allowedVOs += "(";
            for (iterVO = voNameList.begin(); iterVO != voNameList.end(); ++iterVO) {
                allowedVOs += "'";
                allowedVOs += (*iterVO);
                allowedVOs += "',";
            }
            allowedVOs = allowedVOs.substr(0, allowedVOs.size() - 1);
            allowedVOs += ")";
            boost::algorithm::to_lower(allowedVOs);
        } else {
            allowedVOs = voNameList[0];
        }
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
    SiteName siteResolver;
    std::string ftsHostName;
    std::string allowedVOs;
    std::vector<int> requestIDs;
    std::vector<TransferJobs*> jobs2;
    int maximumThreads;
    std::string infosys;

    void killRunninfJob(std::vector<int>& requestIDs) {
        std::vector<int>::const_iterator iter;
        for (iter = requestIDs.begin(); iter != requestIDs.end(); ++iter) {
            int pid = *iter;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Canceling and killing running processes: " << pid << commit;
            kill(pid, SIGTERM);
        }
    }

    std::string extractHostname(std::string surl) {
        std::string hostname("");
        char *base_scheme = NULL;
        char *base_host = NULL;
        char *base_path = NULL;
        int base_port = 0;
        parse_url(surl.c_str(), &base_scheme, &base_host, &base_port, &base_path);
        if (base_host)
            hostname = std::string(base_host);
        if (base_scheme)
            free(base_scheme);
        if (base_host)
            free(base_host);
        if (base_path)
            free(base_path);

        return hostname;
    }

    void createJobFile(std::string job_id, std::vector<std::string>& files) {
        std::vector<std::string>::const_iterator iter;
        std::string filename = "/var/lib/fts3/" + job_id;
        std::ofstream fout;
        fout.open(filename.c_str(), ios::out);
        for (iter = files.begin(); iter != files.end(); ++iter) {
            fout << *iter << std::endl;
        }
        fout.close();
    }

    void executeUrlcopy(std::vector<TransferJobs*>& jobs2, bool reuse) {
        const std::string cmd = "fts_url_copy";
        std::string params = std::string("");
        ExecuteProcess *pr = NULL;
        std::string sourceSiteName("");
        std::string destSiteName("");
        std::string source_hostname("");
        std::string destin_hostname("");
        SeProtocolConfig protocol;
        bool protocolExists;
        std::string proxy_file("");
        bool debug = false;
        OptimizerSample* opt_config = NULL;

        if (reuse == false) {
            bool manualConfigExists = false;
            std::string symbolicName("");
            if (!jobs2.empty()) {
                /*get the file for each job*/
                std::vector<TransferJobs*>::const_iterator iter2;

                std::map< std::string, std::list<TransferFiles*> > voQueues;
                DBSingleton::instance().getDBObjectInstance()->getByJobId(jobs2, voQueues);

                // create transfer-file handler
                TransferFileHandler tfh(voQueues);

                // loop until all files have been served
                while (!tfh.empty()) {

                    // iterate over all VOs
                    set<string>::iterator it_vo;
                    for (it_vo = tfh.begin(); it_vo != tfh.end(); it_vo++) {

                        if (stopThreads) {
                            /** cleanup resources */
                            for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                                delete *iter2;
                            jobs2.clear();
                            return;
                        }

                        int currentActiveTransfers = DBSingleton::instance().getDBObjectInstance()->activeProcessesForThisHost();
                        if (maximumThreads != -1 && currentActiveTransfers != 0 && (currentActiveTransfers * 11) >= maximumThreads) {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Enforced soft limits, currently " << currentActiveTransfers << " are running" << commit;
                            sleep(1);
                            continue;
                        }

                        int freeRam = getAvailableMemory();    			                
                        if (freeRam != -1 && freeRam < 200) {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Enforced limits, free RAM is " << freeRam << "MB and " << currentActiveTransfers << " are running" << commit;
                            sleep(1);
                            continue;
                        }


                        int BufSize = 0;
                        int StreamsperFile = 0;
                        int Timeout = 0;
                        std::stringstream internalParams;
                        // serve the first file from the queue
                        boost::scoped_ptr<TransferFiles> temp(tfh.get(*it_vo));
                        // if there are no more files for that VO just continue
                        if (!temp.get()) continue;

                        source_hostname = temp->SOURCE_SE;
                        destin_hostname = temp->DEST_SE;

                        /*check if manual config exist for this pair and vo*/

                        vector< scoped_ptr<ShareConfig> > cfgs;

                        ConfigurationAssigner cfgAssigner(temp.get());
                        cfgAssigner.assign(cfgs);

                        bool optimize = false;
                        if (enableOptimization.compare("true") == 0 && cfgs.empty()) {
                            optimize = true;
                            opt_config = new OptimizerSample();
                            DBSingleton::instance().getDBObjectInstance()->initOptimizer(source_hostname, destin_hostname, 0);
                            DBSingleton::instance().getDBObjectInstance()->fetchOptimizationConfig2(opt_config, source_hostname, destin_hostname);
                            BufSize = opt_config->getBufSize();
                            StreamsperFile = opt_config->getStreamsperFile();
                            Timeout = opt_config->getTimeout();
                            delete opt_config;
                            opt_config = NULL;
                        }

                        FileTransferScheduler scheduler(temp.get(), cfgs);
                        if (scheduler.schedule(optimize)) { /*SET TO READY STATE WHEN TRUE*/
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer start: " << source_hostname << " -> " << destin_hostname << commit;
                            if (optimize && cfgs.empty()) {
                                DBSingleton::instance().getDBObjectInstance()->setAllowed(temp->JOB_ID, temp->FILE_ID, source_hostname, destin_hostname, StreamsperFile, Timeout, BufSize);
                            } else {
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << commit;
                                ProtocolResolver resolver(temp.get(), cfgs);
                                protocolExists = resolver.resolve();
                                if (protocolExists) {

                                    protocol.NOSTREAMS = resolver.getNoStreams();
                                    protocol.NO_TX_ACTIVITY_TO = resolver.getNoTxActiveTo();
                                    protocol.TCP_BUFFER_SIZE = resolver.getTcpBufferSize();
                                    protocol.URLCOPY_TX_TO = resolver.getUrlCopyTxTo();

                                    if (protocol.NOSTREAMS >= 0)
                                        internalParams << "nostreams:" << protocol.NOSTREAMS;
                                    if (protocol.URLCOPY_TX_TO >= 0)
                                        internalParams << ",timeout:" << protocol.URLCOPY_TX_TO;
                                    if (protocol.TCP_BUFFER_SIZE >= 0)
                                        internalParams << ",buffersize:" << protocol.TCP_BUFFER_SIZE;
                                } else {
                                    internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                                }

                                if (resolver.isAuto()) {
                                    DBSingleton::instance().getDBObjectInstance()->setAllowed(
                                            temp->JOB_ID,
                                            temp->FILE_ID,
                                            source_hostname,
                                            destin_hostname,
                                            resolver.getNoStreams(),
                                            resolver.getNoTxActiveTo(),
                                            resolver.getTcpBufferSize()
                                            );
                                } else {
                                    DBSingleton::instance().getDBObjectInstance()->setAllowedNoOptimize(
                                            temp->JOB_ID,
                                            temp->FILE_ID,
                                            internalParams.str()
                                            );
                                }
                            }

                            proxy_file = get_proxy_cert(
                                    temp->DN, // user_dn
                                    temp->CRED_ID, // user_cred
                                    temp->VO_NAME, // vo_name
                                    "",
                                    "", // assoc_service
                                    "", // assoc_service_type
                                    false,
                                    ""
                                    );

                            sourceSiteName = siteResolver.getSiteName(temp->SOURCE_SURL);
                            destSiteName = siteResolver.getSiteName(temp->DEST_SURL);

                            debug = DBSingleton::instance().getDBObjectInstance()->getDebugMode(source_hostname, destin_hostname);

                            if (debug == true) {
                                params.append(" -F ");
                            }

                            if (proxy_file.length() > 0) {
                                params.append(" -proxy ");
                                params.append(proxy_file);
                                /*make sure proxy is readable    */
                                chmod(proxy_file.c_str(), (mode_t) 0600); //S_IRUSR|S_IRGRP|S_IROTH                            
                                uid_t pw_uid;
                                pw_uid = name_to_uid();
                                int checkChown = chown(proxy_file.c_str(), pw_uid, getgid());
                                if (checkChown != 0) {
                                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chmod for proxy" << proxy_file << commit;
                                }

                            }

                            if (std::string(temp->CHECKSUM).length() > 0) { //checksum
                                params.append(" -z ");
                                params.append(temp->CHECKSUM);
                            }
                            if (std::string(temp->CHECKSUM_METHOD).length() > 0) { //checksum
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
                            if (std::string(temp->OVERWRITE).length() > 0) {
                                params.append(" -d ");
                            }
                            if (optimize && manualConfigExists == false) {
                                params.append(" -e ");
                                params.append(to_string(StreamsperFile));
                            } else {
                                if (protocol.NOSTREAMS >= 0) {
                                    params.append(" -e ");
                                    params.append(to_string(protocol.NOSTREAMS));
                                }
                            }

                            if (optimize && manualConfigExists == false) {
                                params.append(" -f ");
                                params.append(to_string(BufSize));
                            } else {
                                if (protocol.TCP_BUFFER_SIZE >= 0) {
                                    params.append(" -f ");
                                    params.append(to_string(protocol.TCP_BUFFER_SIZE));
                                }
                            }

                            if (optimize && manualConfigExists == false) {
                                params.append(" -h ");
                                params.append(to_string(Timeout));
                            } else {
                                if (protocol.URLCOPY_TX_TO >= 0) {
                                    params.append(" -h ");
                                    params.append(to_string(protocol.URLCOPY_TX_TO));
                                }
                            }
                            if (std::string(temp->SOURCE_SPACE_TOKEN).length() > 0) {
                                params.append(" -k ");
                                params.append(temp->SOURCE_SPACE_TOKEN);
                            }
                            if (std::string(temp->DEST_SPACE_TOKEN).length() > 0) {
                                params.append(" -j ");
                                params.append(temp->DEST_SPACE_TOKEN);
                            }

                            if (temp->PIN_LIFETIME > 0) {
                                params.append(" -t ");
                                params.append(to_string(temp->PIN_LIFETIME));
                            }

                            if (temp->BRINGONLINE > 0) {
                                params.append(" -H ");
                                params.append(to_string(temp->BRINGONLINE));
                            }

                            if (temp->USER_FILESIZE > 0) {
                                params.append(" -I ");
                                params.append(to_string(temp->USER_FILESIZE));
                            }

                            if (temp->FILE_METADATA.length() > 0) {
                                params.append(" -K ");
                                params.append(temp->FILE_METADATA);
                            }

                            if (temp->JOB_METADATA.length() > 0) {
                                params.append(" -J ");
                                params.append(temp->JOB_METADATA);
                            }

                            if (temp->BRINGONLINE_TOKEN.length() > 0) {
                                params.append(" -L ");
                                params.append(temp->BRINGONLINE_TOKEN);
                            }
			    
                             params.append(" -M ");
                             params.append(infosys);


                            std::string host = DBSingleton::instance().getDBObjectInstance()->transferHost(temp->FILE_ID);
                            bool ready = DBSingleton::instance().getDBObjectInstance()->isFileReadyState(temp->FILE_ID);

                            if (host.compare(ftsHostName) == 0 && ready == true) {
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                                pr = new ExecuteProcess(cmd, params, 0);
                                if (pr) {
                                    /*check if fork/execvp failed, */
                                    if (-1 == pr->executeProcessShell()) {
                                        DBSingleton::instance().getDBObjectInstance()->forkFailedRevertState(temp->JOB_ID, temp->FILE_ID);
                                    } else {
                                        DBSingleton::instance().getDBObjectInstance()->setPid(temp->JOB_ID, temp->FILE_ID, pr->getPid());
                                        struct message_updater msg;
                                        strcpy(msg.job_id, std::string(temp->JOB_ID).c_str());
                                        msg.file_id = temp->FILE_ID;
                                        msg.process_id = (int) pr->getPid();
                                        msg.timestamp = milliseconds_since_epoch();
                                        ThreadSafeList::get_instance().push_back(msg);
                                    }
                                    delete pr;
                                }
                            }
                            params.clear();
                        }
                    }
                }

                /** cleanup resources */
                for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                    delete *iter2;
                jobs2.clear();
            }
        } else { /*reuse session*/
            if (!jobs2.empty()) {
                bool manualConfigExists = false;
                std::string symbolicName("");
                std::vector<std::string> urls;
                std::map<int, std::string> fileIds;
                std::string job_id = std::string("");
                std::string vo_name = std::string("");
                std::string cred_id = std::string("");
                std::string dn = std::string("");
                std::string overwrite = std::string("");
                std::string source_space_token = std::string("");
                std::string dest_space_token = std::string("");
                int file_id = 0;
                std::string checksum = std::string("");
                std::stringstream url;
                std::string surl = std::string("");
                std::string durl = std::string("");
                int pinLifetime = -1;
                int bringOnline = -1;
                int BufSize = 0;
                int StreamsperFile = 0;
                int Timeout = 0;
                double userFilesize = 0;
                std::string jobMetadata("");
                std::string fileMetadata("");
		std::string bringonlineToken("");

                TransferFiles* tempUrl = NULL;
                /*get the file for each job*/
                std::vector<TransferJobs*>::const_iterator iter2;

                std::map< std::string, std::list<TransferFiles*> > voQueues;
                std::list<TransferFiles*>::const_iterator queueiter;

                DBSingleton::instance().getDBObjectInstance()->getByJobId(jobs2, voQueues);

                if (voQueues.empty()) {
                    /** cleanup resources */
                    for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                        delete *iter2;
                    jobs2.clear();
                    return;
                }

                // since there will be just one VO pick it (TODO)
                std::string vo = jobs2.front()->VO_NAME;

                for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter) {
                    if (stopThreads) {
                        return;
                    }


                    int currentActiveTransfers = DBSingleton::instance().getDBObjectInstance()->activeProcessesForThisHost();
                    if (maximumThreads != -1 && currentActiveTransfers != 0 && (currentActiveTransfers * 11) >= maximumThreads) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Enforced soft limits, currently " << currentActiveTransfers << " are running" << commit;
                        sleep(1);
                        continue;
                    }

                    int freeRam = getAvailableMemory();                    
                    if (freeRam != -1 && freeRam < 200) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Enforced limits, free RAM is " << freeRam << "MB and " << currentActiveTransfers << " are running" << commit;
                        sleep(1);
                        continue;
                    }


                    TransferFiles* temp = (TransferFiles*) * queueiter;
                    tempUrl = temp;
                    surl = temp->SOURCE_SURL;
                    durl = temp->DEST_SURL;
                    job_id = temp->JOB_ID;
                    vo_name = temp->VO_NAME;
                    cred_id = temp->CRED_ID;
                    dn = temp->DN;
                    file_id = temp->FILE_ID;
                    overwrite = temp->OVERWRITE;
                    source_hostname = extractHostname(temp->SOURCE_SURL);
                    destin_hostname = extractHostname(temp->DEST_SURL);
                    source_space_token = temp->SOURCE_SPACE_TOKEN;
                    dest_space_token = temp->DEST_SPACE_TOKEN;
                    pinLifetime = temp->PIN_LIFETIME;
                    bringOnline = temp->BRINGONLINE;
                    userFilesize = temp->USER_FILESIZE;
                    jobMetadata = temp->JOB_METADATA;
                    fileMetadata = temp->FILE_METADATA;
		    bringonlineToken = temp->BRINGONLINE_TOKEN;

                    if (fileMetadata.length() <= 0)
                        fileMetadata = "x";
		    if (bringonlineToken.length() <= 0)
                        bringonlineToken = "x";

                    if (std::string(temp->CHECKSUM_METHOD).length() > 0) {
                        if (std::string(temp->CHECKSUM).length() > 0)
                            checksum = temp->CHECKSUM;
                    }
                    url << file_id << " " << surl << " " << durl << " " << checksum << " " << userFilesize << " " << fileMetadata << " " << bringonlineToken;
                    urls.push_back(url.str());
                    url.str("");
                }

                sourceSiteName = siteResolver.getSiteName(surl);
                destSiteName = siteResolver.getSiteName(durl);

                createJobFile(job_id, urls);

                /*check if manual config exist for this pair and vo*/
                vector< scoped_ptr<ShareConfig> > cfgs;
                ConfigurationAssigner cfgAssigner(tempUrl);
                cfgAssigner.assign(cfgs);

                bool optimize = false;

                if (enableOptimization.compare("true") == 0 && cfgs.empty()) {
                    optimize = true;
                    opt_config = new OptimizerSample();
                    DBSingleton::instance().getDBObjectInstance()->initOptimizer(source_hostname, destin_hostname, 0);
                    DBSingleton::instance().getDBObjectInstance()->fetchOptimizationConfig2(opt_config, source_hostname, destin_hostname);
                    BufSize = opt_config->getBufSize();
                    StreamsperFile = opt_config->getStreamsperFile();
                    Timeout = opt_config->getTimeout();
                    delete opt_config;
                    opt_config = NULL;
                }


                FileTransferScheduler scheduler(tempUrl, cfgs);
                if (scheduler.schedule(optimize)) { /*SET TO READY STATE WHEN TRUE*/
                    std::stringstream internalParams;
                    if (optimize && manualConfigExists == false) {
                        DBSingleton::instance().getDBObjectInstance()->setAllowed(job_id, -1, source_hostname, destin_hostname, StreamsperFile, Timeout, BufSize);
                    } else {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << " -> " << vo_name << commit;
                        ProtocolResolver resolver(tempUrl, cfgs);
                        protocolExists = resolver.resolve();
                        if (protocolExists) {

                            protocol.NOSTREAMS = resolver.getNoStreams();
                            protocol.NO_TX_ACTIVITY_TO = resolver.getNoTxActiveTo();
                            protocol.TCP_BUFFER_SIZE = resolver.getTcpBufferSize();
                            protocol.URLCOPY_TX_TO = resolver.getUrlCopyTxTo();

                            if (protocol.NOSTREAMS >= 0)
                                internalParams << "nostreams:" << protocol.NOSTREAMS;
                            if (protocol.URLCOPY_TX_TO >= 0)
                                internalParams << ",timeout:" << protocol.URLCOPY_TX_TO;
                            if (protocol.TCP_BUFFER_SIZE >= 0)
                                internalParams << ",buffersize:" << protocol.TCP_BUFFER_SIZE;
                        } else {
                            internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                        }

                        if (resolver.isAuto()) {
                            DBSingleton::instance().getDBObjectInstance()->setAllowed(
                                    job_id,
                                    -1,
                                    source_hostname,
                                    destin_hostname,
                                    resolver.getNoStreams(),
                                    resolver.getNoTxActiveTo(),
                                    resolver.getTcpBufferSize()
                                    );
                        } else {
                            DBSingleton::instance().getDBObjectInstance()->setAllowedNoOptimize(
                                    job_id,
                                    0,
                                    internalParams.str()
                                    );
                        }
                    }

                    proxy_file = get_proxy_cert(
                            dn, // user_dn
                            cred_id, // user_cred
                            vo_name, // vo_name
                            "",
                            "", // assoc_service
                            "", // assoc_service_type
                            false,
                            "");

                    /*set all to ready, special case for session reuse*/
                    for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter) {
                        TransferFiles* temp = (TransferFiles*) * queueiter;
                        DBSingleton::instance().getDBObjectInstance()->updateFileStatus(temp, "READY");
                        fileIds.insert(std::make_pair<int, std::string > (temp->FILE_ID, temp->JOB_ID));
                    }

                    debug = DBSingleton::instance().getDBObjectInstance()->getDebugMode(source_hostname, destin_hostname);
                    if (debug == true) {
                        params.append(" -F ");
                    }

                    if (proxy_file.length() > 0) {
                        params.append(" -proxy ");
                        params.append(proxy_file);
                        /*make sure proxy is readable    */
                        chmod(proxy_file.c_str(), (mode_t) 0600);
                        uid_t pw_uid;
                        pw_uid = name_to_uid();
                        int checkChown = chown(proxy_file.c_str(), pw_uid, getgid());
                        if (checkChown != 0) {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to chmod for proxy" << proxy_file << commit;
                        }
                    }

                    params.append(" -G ");
                    params.append(" -a ");
                    params.append(job_id);
                    params.append(" -C ");
                    params.append(vo_name);
                    if (sourceSiteName.length() > 0) {
                        params.append(" -D ");
                        params.append(sourceSiteName);
                    }
                    if (destSiteName.length() > 0) {
                        params.append(" -E ");
                        params.append(destSiteName);
                    }
                    if (std::string(overwrite).length() > 0) {
                        params.append(" -d ");
                    }
                    if (optimize && manualConfigExists == false) {
                        params.append(" -e ");
                        params.append(to_string(StreamsperFile));
                    } else {
                        if (protocol.NOSTREAMS >= 0) {
                            params.append(" -e ");
                            params.append(to_string(protocol.NOSTREAMS));
                        }
                    }
                    if (optimize && manualConfigExists == false) {
                        params.append(" -f ");
                        params.append(to_string(BufSize));
                    } else {
                        if (protocol.TCP_BUFFER_SIZE >= 0) {
                            params.append(" -f ");
                            params.append(to_string(protocol.TCP_BUFFER_SIZE));
                        }
                    }
                    if (optimize && manualConfigExists == false) {
                        params.append(" -h ");
                        params.append(to_string(Timeout));
                    } else {
                        if (protocol.URLCOPY_TX_TO >= 0) {
                            params.append(" -h ");
                            params.append(to_string(protocol.URLCOPY_TX_TO));
                        }
                    }
                    if (std::string(source_space_token).length() > 0) {
                        params.append(" -k ");
                        params.append(source_space_token);
                    }
                    if (std::string(dest_space_token).length() > 0) {
                        params.append(" -j ");
                        params.append(dest_space_token);
                    }

                    if (pinLifetime > 0) {
                        params.append(" -t ");
                        params.append(to_string(pinLifetime));
                    }

                    if (bringOnline > 0) {
                        params.append(" -H ");
                        params.append(to_string(bringOnline));
                    }


                    if (userFilesize > 0) {
                        params.append(" -I ");
                        params.append(to_string(userFilesize));
                    }

                    if (jobMetadata.length() > 0) {
                        params.append(" -J ");
                        params.append(jobMetadata);
                    }
		    
                    params.append(" -M ");
                    params.append(infosys);		    

                    std::string host = DBSingleton::instance().getDBObjectInstance()->transferHostV(fileIds);
                    bool ready = DBSingleton::instance().getDBObjectInstance()->isFileReadyStateV(fileIds);

                    if (host.compare(ftsHostName) == 0 && ready == true) {
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                        pr = new ExecuteProcess(cmd, params, 0);
                        if (pr) {
                            /*check if fork failed , check if execvp failed, */
                            if (-1 == pr->executeProcessShell()) {
                                DBSingleton::instance().getDBObjectInstance()->forkFailedRevertStateV(fileIds);
                            } else {
                                DBSingleton::instance().getDBObjectInstance()->setPidV(pr->getPid(), fileIds);
                                std::map<int, std::string>::const_iterator iterFileIds;
                                for (iterFileIds = fileIds.begin(); iterFileIds != fileIds.end(); ++iterFileIds) {
                                    struct message_updater msg;
                                    strcpy(msg.job_id, std::string(job_id).c_str());
                                    msg.file_id = iterFileIds->first;
                                    msg.process_id = (int) pr->getPid();
                                    msg.timestamp = std::time(NULL);
                                    ThreadSafeList::get_instance().push_back(msg);
                                }
                            }
                            delete pr;
                        }
                    }
                    params.clear();
                }


                /** cleanup resources */
                for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                    delete *iter2;
                jobs2.clear();
                for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                    delete *queueiter;
                voQueues[vo].clear();
                fileIds.clear();

            }
        }
    }

    /* ---------------------------------------------------------------------- */
    void executeTransfer_a() {
        static bool drainMode = false;
        static long double counter = 0;
        static unsigned int countReverted = 0;

        while (1) {
            try {
                if (stopThreads) {
                    if (!jobs2.empty()) {
                        std::vector<TransferJobs*>::const_iterator iter2;
                        for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                            delete *iter2;
                        jobs2.clear();
                    }
                    return;
                }

                if (DrainMode::getInstance()) {
                    if (!drainMode)
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                    drainMode = true;
                    continue;
                } else {
                    drainMode = false;
                }

                /*force fail to stall transfers*/
                counter++;
                if (counter == 2000) {
                    DBSingleton::instance().getDBObjectInstance()->forceFailTransfers();
                    counter = 0;
                }

                /*revert to SUBMITTED state if stayed in READY for too long*/
                countReverted++;
                if (countReverted == 100) {
                    DBSingleton::instance().getDBObjectInstance()->revertToSubmitted();
                    countReverted = 0;
                }

                /*get jobs in submitted state*/
                DBSingleton::instance().getDBObjectInstance()->getSubmittedJobs(jobs2, allowedVOs);

                /*also get jobs which have been canceled by the client*/
                DBSingleton::instance().getDBObjectInstance()->getCancelJob(requestIDs);
                if (!requestIDs.empty()) { /*if canceled jobs found and transfer already started, kill them*/
                    killRunninfJob(requestIDs);
                    requestIDs.clear(); /*clean the list*/
                }

                if (!jobs2.empty())
                    executeUrlcopy(jobs2, false);

                /* --- session reuse section ---*/
                /*get jobs in submitted state and session reuse on*/
                DBSingleton::instance().getDBObjectInstance()->getSubmittedJobsReuse(jobs2, allowedVOs);
                if (!jobs2.empty())
                    executeUrlcopy(jobs2, true);

                if (!jobs2.empty()) {
                    std::vector<TransferJobs*>::const_iterator iter2;
                    for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                        delete *iter2;
                    jobs2.clear();
                }

            } catch (...) {
                if (!jobs2.empty()) {
                    std::vector<TransferJobs*>::const_iterator iter2;
                    for (iter2 = jobs2.begin(); iter2 != jobs2.end(); ++iter2)
                        delete *iter2;
                    jobs2.clear();
                }
            }
            usleep(10000);
        } /*end while*/
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

