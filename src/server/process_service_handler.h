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
#include "FileTransferExecutorPool.h"
#include "TransferFileHandler.h"
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
#include <boost/shared_ptr.hpp>
#include "name_to_uid.h"
#include "producer_consumer_common.h"
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <boost/algorithm/string/replace.hpp>
#include "ws/SingleTrStateInstance.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include "profiler/Profiler.h"
#include "profiler/Macros.h"
#include <boost/thread.hpp>

extern bool stopThreads;


FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

int proc_find(const char* name)
{
    DIR* dir=NULL;
    struct dirent* ent=NULL;
    char* endptr=NULL;
    char buf[512]= {0};
    unsigned count = 0;

    if (!(dir = opendir("/proc")))
        {
            return -1;
        }

    while((ent = readdir(dir)) != NULL)
        {
            /* if endptr is not a null character, the directory is not
             * entirely numeric, so ignore it */
            long lpid = strtol(ent->d_name, &endptr, 10);
            if (*endptr != '\0')
                {
                    continue;
                }

            /* try to open the cmdline file */
            snprintf(buf, sizeof(buf), "/proc/%ld/cmdline", lpid);
            FILE* fp = fopen(buf, "r");

            if (fp)
                {
                    if (fgets(buf, sizeof(buf), fp) != NULL)
                        {
                            /* check the first token in the file, the program name */
                            char* first = NULL;
                            first = strtok(buf, " ");

                            if (first && !strcmp(first, name))
                                {
                                    fclose(fp);
                                    fp = NULL;
                                    ++count;
                                }
                        }
                    if(fp)
                        fclose(fp);
                }

        }

    closedir(dir);
    return count;
}



/*resource resource management as not to run out of memory or too many processes*/
static rlim_t getMaxThreads()
{
    struct rlimit rlim;
    int err = -1;
    err = getrlimit(RLIMIT_NPROC, &rlim);
    if (0 != err)
        {
            return 0;
        }
    else
        {
            return rlim.rlim_cur;
        }
    return 0;
}


static long unsigned int getAvailableMemory()
{
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        return 0;

    return (info.freeram) / (1024 * 1024);
}


static std::string prepareMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, " ", "?");
    text = boost::replace_all_copy(text, "\"", "\\\"");
    return text;
}


template
<
typename TRAITS
>
class ProcessServiceHandler : public TRAITS::ActiveObjectType
{
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
        TRAITS::ActiveObjectType("ProcessServiceHandler", desc)
    {
        cmd = "fts_url_copy";
        maximumThreads = getMaxThreads();

        execPoolSize = theServerConfig().get<int> ("InternalThreadPool");

        enableOptimization = theServerConfig().get<std::string > ("Optimizer");
        char hostname[MAXHOSTNAMELEN];
        gethostname(hostname, MAXHOSTNAMELEN);
        ftsHostName = std::string(hostname);
        allowedVOs = std::string("");
        infosys = theServerConfig().get<std::string > ("Infosys");
        const vector<std::string> voNameList(theServerConfig().get< vector<string> >("AuthorizedVO"));
        if (voNameList.size() > 0 && std::string(voNameList[0]).compare("*") != 0)
            {
                std::vector<std::string>::const_iterator iterVO;
                allowedVOs += "(";
                for (iterVO = voNameList.begin(); iterVO != voNameList.end(); ++iterVO)
                    {
                        allowedVOs += "'";
                        allowedVOs += (*iterVO);
                        allowedVOs += "',";
                    }
                allowedVOs = allowedVOs.substr(0, allowedVOs.size() - 1);
                allowedVOs += ")";
                boost::algorithm::to_lower(allowedVOs);
            }
        else
            {
                allowedVOs = voNameList[0];
            }

        std::string monitoringMessagesStr = theServerConfig().get<std::string > ("MonitoringMessaging");
        if(monitoringMessagesStr == "false")
            monitoringMessages = false;
        else
            monitoringMessages = true;

    }

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessServiceHandler()
    {
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p
    (
    )
    {

        boost::function<void() > op = boost::bind(&ProcessServiceHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:
    SiteName siteResolver;
    std::string ftsHostName;
    std::string allowedVOs;
    std::vector<TransferJobs*> jobsReuse;
    rlim_t maximumThreads;
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string cmd;
    boost::thread_group g;
    mutable ThreadTraits::MUTEX_R _mutex;


    std::string extractHostname(const std::string &surl)
    {
        Uri u0 = Uri::Parse(surl);
        return u0.Protocol + "://" + u0.Host;
    }

    void createJobFile(std::string job_id, std::vector<std::string>& files)
    {
        std::vector<std::string>::const_iterator iter;
        std::string filename = "/var/lib/fts3/" + job_id;
        std::ofstream fout;
        fout.open(filename.c_str(), ios::out);
        for (iter = files.begin(); iter != files.end(); ++iter)
            {
                fout << *iter << std::endl;
            }
        fout.close();
    }

    void fetchFiles(std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFiles*> >& voQueues)
    {
        ThreadTraits::LOCK_R lock(_mutex);
        try
            {
                DBSingleton::instance().getDBObjectInstance()->getByJobId(distinct, voQueues);
            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in fetchFiles " << e.what() << commit;
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in fetchFiles!" << commit;
            }
    }

    void executeUrlcopy(std::vector<TransferJobs*>& jobsReuse2, bool reuse)
    {
        try
            {
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

                if (reuse == false)
                    {
                        std::map< std::string, std::list<TransferFiles*> > voQueues;
                        std::vector< boost::tuple<std::string, std::string, std::string> > distinct = DBSingleton::instance().getDBObjectInstance()->distinctSrcDestVO();
                        if(distinct.empty())
                            return;

                        if(distinct.size() >= 4 )
                            {
                                std::size_t const half_size1 = distinct.size() / 2;
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_1(distinct.begin(), distinct.begin() + half_size1);
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_2(distinct.begin() + half_size1, distinct.end());

                                std::size_t const half_size2 = split_1.size() / 2;
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_11(split_1.begin(), split_1.begin() + half_size2);
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_21(split_1.begin() + half_size2, split_1.end());

                                std::size_t const half_size3 = split_2.size() / 2;
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_12(split_2.begin(), split_2.begin() + half_size3);
                                std::vector< boost::tuple<std::string, std::string, std::string> > split_22(split_2.begin() + half_size3, split_2.end());

                                boost::thread *t1 = new boost::thread(boost::bind(&ProcessServiceHandler::fetchFiles, this, boost::ref(split_11),boost::ref(voQueues)));
                                boost::thread *t2 = new boost::thread(boost::bind(&ProcessServiceHandler::fetchFiles, this, boost::ref(split_21),boost::ref(voQueues)));
                                boost::thread *t3 = new boost::thread(boost::bind(&ProcessServiceHandler::fetchFiles, this, boost::ref(split_12),boost::ref(voQueues)));
                                boost::thread *t4 = new boost::thread(boost::bind(&ProcessServiceHandler::fetchFiles, this, boost::ref(split_22),boost::ref(voQueues)));

                                g.add_thread(t1);
                                g.add_thread(t2);
                                g.add_thread(t3);
                                g.add_thread(t4);

                                // wait for them
                                g.join_all();
                            }
                        else    //end < 4
                            {
                                fetchFiles(distinct, voQueues);
                            }

                        // create transfer-file handler
                        TransferFileHandler tfh(voQueues);

                        //init optimizer here to avoid duplicates
                        std::map< std::string, std::list<TransferFiles*> >::const_iterator i;
                        for (i = voQueues.begin(); i != voQueues.end(); ++i)
                            {
                                std::list<TransferFiles*>::const_iterator j;
                                for (j = i->second.begin(); j != i->second.end(); ++j)
                                    {
                                        TransferFiles* temp = *j;
                                        DBSingleton::instance().getDBObjectInstance()->initOptimizer(temp->SOURCE_SE, temp->DEST_SE, 0);
                                    }
                            }

                        // the worker thread pool
                        FileTransferExecutorPool execPool(execPoolSize, tfh, monitoringMessages, infosys, ftsHostName);

                        // loop until all files have been served

                        int initial_size = tfh.size();


                        while (!tfh.empty())
                            {
                                PROFILE_SCOPE("executeUrlcopy::while[!reuse]");

                                // iterate over all VOs
                                set<string>::iterator it_vo;
                                for (it_vo = tfh.begin(); it_vo != tfh.end(); it_vo++)
                                    {
                                        if (stopThreads)
                                            {
                                                execPool.stopAll();
                                                return;
                                            }

                                        execPool.add(tfh.get(*it_vo), enableOptimization.compare("true") == 0);
                                    }
                            }

                        // wait for all the workers to finish
                        execPool.join();

                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Threadpool processed: " << initial_size << " files (" << execPool.getNumberOfScheduled() << " have been scheduled)" << commit;
                    }
                else     /*reuse session*/
                    {
                        if (!jobsReuse2.empty())
                            {
                                bool manualConfigExists = false;
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

                                std::map< std::string, std::list<TransferFiles*> > voQueues;
                                std::list<TransferFiles*>::const_iterator queueiter;

                                DBSingleton::instance().getDBObjectInstance()->getByJobIdReuse(jobsReuse2, voQueues, reuse);

                                if (voQueues.empty())
                                    {
                                        std::vector<TransferJobs*>::iterator iter2;
                                        for (iter2 = jobsReuse2.begin(); iter2 != jobsReuse2.end(); ++iter2)
                                            {
                                                if(*iter2)
                                                    delete *iter2;
                                            }
                                        jobsReuse2.clear();
                                        return;
                                    }

                                // since there will be just one VO pick it (TODO)
                                std::string vo = jobsReuse2.front()->VO_NAME;

                                for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                    {
                                        PROFILE_SCOPE("executeUrlcopy::for[reuse]");
                                        if (stopThreads)
                                            {
                                                return;
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
                                        source_hostname = temp->SOURCE_SE;
                                        destin_hostname = temp->DEST_SE;
                                        source_space_token = temp->SOURCE_SPACE_TOKEN;
                                        dest_space_token = temp->DEST_SPACE_TOKEN;
                                        pinLifetime = temp->PIN_LIFETIME;
                                        bringOnline = temp->BRINGONLINE;
                                        userFilesize = temp->USER_FILESIZE;
                                        jobMetadata = prepareMetadataString(temp->JOB_METADATA);
                                        fileMetadata = prepareMetadataString(temp->FILE_METADATA);
                                        bringonlineToken = temp->BRINGONLINE_TOKEN;

                                        if (fileMetadata.length() <= 0)
                                            fileMetadata = "x";
                                        if (bringonlineToken.length() <= 0)
                                            bringonlineToken = "x";
                                        if (std::string(temp->CHECKSUM_METHOD).length() > 0)
                                            {
                                                if (std::string(temp->CHECKSUM).length() > 0)
                                                    checksum = temp->CHECKSUM;
                                                else
                                                    checksum = "x";
                                            }
                                        else
                                            {
                                                checksum = "x";
                                            }

                                        url << std::fixed << file_id << " " << surl << " " << durl << " " << checksum << " " << userFilesize << " " << fileMetadata << " " << bringonlineToken;
                                        urls.push_back(url.str());
                                        url.str("");
                                    }

                                if(!tempUrl)
                                    {
                                        /** cleanup resources */
                                        std::vector<TransferJobs*>::iterator iter2;
                                        for (iter2 = jobsReuse2.begin(); iter2 != jobsReuse2.end(); ++iter2)
                                            {
                                                if(*iter2)
                                                    delete *iter2;
                                            }
                                        jobsReuse2.clear();
                                        for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                            {
                                                if(*queueiter)
                                                    delete *queueiter;
                                            }
                                        voQueues[vo].clear();
                                        fileIds.clear();
                                        return;
                                    }

                                sourceSiteName = siteResolver.getSiteName(surl);
                                destSiteName = siteResolver.getSiteName(durl);

                                createJobFile(job_id, urls);

                                /*check if manual config exist for this pair and vo*/
                                vector< boost::shared_ptr<ShareConfig> > cfgs;
                                ConfigurationAssigner cfgAssigner(tempUrl);
                                cfgAssigner.assign(cfgs);

                                bool optimize = false;

                                if (enableOptimization.compare("true") == 0 && cfgs.empty())
                                    {
                                        optimize = true;
                                        opt_config = new OptimizerSample();
                                        DBSingleton::instance().getDBObjectInstance()->fetchOptimizationConfig2(opt_config, source_hostname, destin_hostname);
                                        BufSize = opt_config->getBufSize();
                                        StreamsperFile = opt_config->getStreamsperFile();
                                        Timeout = opt_config->getTimeout();
                                        delete opt_config;
                                        opt_config = NULL;
                                    }
                                else
                                    {
                                        manualConfigExists = true;
                                    }

                                FileTransferScheduler scheduler(tempUrl, cfgs);
                                if (scheduler.schedule(optimize))   /*SET TO READY STATE WHEN TRUE*/
                                    {
                                        bool isAutoTuned = false;
                                        std::stringstream internalParams;
                                        if (optimize && manualConfigExists == false)
                                            {
                                                DBSingleton::instance().getDBObjectInstance()->setAllowed(job_id, -1, source_hostname, destin_hostname, StreamsperFile, Timeout, BufSize);
                                            }
                                        else
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << " -> " << vo_name << commit;
                                                ProtocolResolver resolver(tempUrl, cfgs);
                                                protocolExists = resolver.resolve();
                                                if (protocolExists)
                                                    {
                                                        manualConfigExists = true;
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
                                                    }
                                                else
                                                    {
                                                        internalParams << "nostreams:" << DEFAULT_NOSTREAMS << ",timeout:" << DEFAULT_TIMEOUT << ",buffersize:" << DEFAULT_BUFFSIZE;
                                                    }

                                                if (resolver.isAuto())
                                                    {
                                                        isAutoTuned = true;
                                                        DBSingleton::instance().getDBObjectInstance()->setAllowed(
                                                            job_id,
                                                            -1,
                                                            source_hostname,
                                                            destin_hostname,
                                                            resolver.getNoStreams(),
                                                            resolver.getNoTxActiveTo(),
                                                            resolver.getTcpBufferSize()
                                                        );
                                                    }
                                                else
                                                    {
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
                                        for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                            {
                                                TransferFiles* temp = (TransferFiles*) * queueiter;
                                                DBSingleton::instance().getDBObjectInstance()->updateFileStatus(temp, "READY");
                                                fileIds.insert(std::make_pair<int, std::string > (temp->FILE_ID, temp->JOB_ID));
                                                SingleTrStateInstance::instance().sendStateMessage(temp->JOB_ID, temp->FILE_ID);
                                            }

                                        debug = DBSingleton::instance().getDBObjectInstance()->getDebugMode(source_hostname, destin_hostname);
                                        if (debug == true)
                                            {
                                                params.append(" -F ");
                                            }

                                        if (manualConfigExists)
                                            {
                                                params.append(" -N ");
                                            }

                                        if (isAutoTuned)
                                            {
                                                params.append(" -O ");
                                            }

                                        if (monitoringMessages)
                                            {
                                                params.append(" -P ");
                                            }

                                        if (proxy_file.length() > 0)
                                            {
                                                params.append(" -proxy ");
                                                params.append(proxy_file);
                                            }

                                        params.append(" -G ");
                                        params.append(" -a ");
                                        params.append(job_id);
                                        params.append(" -C ");
                                        params.append(vo_name);
                                        if (sourceSiteName.length() > 0)
                                            {
                                                params.append(" -D ");
                                                params.append(sourceSiteName);
                                            }
                                        if (destSiteName.length() > 0)
                                            {
                                                params.append(" -E ");
                                                params.append(destSiteName);
                                            }
                                        if (std::string(overwrite).length() > 0)
                                            {
                                                params.append(" -d ");
                                            }
                                        if (optimize && manualConfigExists == false)
                                            {
                                                params.append(" -e ");
                                                params.append(boost::lexical_cast<std::string > (StreamsperFile));
                                            }
                                        else
                                            {
                                                if (protocol.NOSTREAMS >= 0)
                                                    {
                                                        params.append(" -e ");
                                                        params.append(boost::lexical_cast<std::string > (protocol.NOSTREAMS));
                                                    }
                                                else
                                                    {
                                                        params.append(" -e ");
                                                        params.append(boost::lexical_cast<std::string > (DEFAULT_NOSTREAMS));
                                                    }
                                            }
                                        if (optimize && manualConfigExists == false)
                                            {
                                                params.append(" -f ");
                                                params.append(boost::lexical_cast<std::string > (BufSize));
                                            }
                                        else
                                            {
                                                if (protocol.TCP_BUFFER_SIZE >= 0)
                                                    {
                                                        params.append(" -f ");
                                                        params.append(boost::lexical_cast<std::string > (protocol.TCP_BUFFER_SIZE));
                                                    }
                                                else
                                                    {
                                                        params.append(" -f ");
                                                        params.append(boost::lexical_cast<std::string > (DEFAULT_BUFFSIZE));
                                                    }
                                            }
                                        if (optimize && manualConfigExists == false)
                                            {
                                                params.append(" -h ");
                                                params.append(boost::lexical_cast<std::string > (Timeout));
                                            }
                                        else
                                            {
                                                if (protocol.URLCOPY_TX_TO >= 0)
                                                    {
                                                        params.append(" -h ");
                                                        params.append(boost::lexical_cast<std::string > (protocol.URLCOPY_TX_TO));
                                                    }
                                                else
                                                    {
                                                        params.append(" -h ");
                                                        params.append(boost::lexical_cast<std::string > (DEFAULT_TIMEOUT));
                                                    }
                                            }
                                        if (std::string(source_space_token).length() > 0)
                                            {
                                                params.append(" -k ");
                                                params.append(source_space_token);
                                            }
                                        if (std::string(dest_space_token).length() > 0)
                                            {
                                                params.append(" -j ");
                                                params.append(dest_space_token);
                                            }

                                        if (pinLifetime > 0)
                                            {
                                                params.append(" -t ");
                                                params.append(boost::lexical_cast<std::string > (pinLifetime));
                                            }

                                        if (bringOnline > 0)
                                            {
                                                params.append(" -H ");
                                                params.append(boost::lexical_cast<std::string > (bringOnline));
                                            }

                                        if (jobMetadata.length() > 0)
                                            {
                                                params.append(" -J ");
                                                params.append(jobMetadata);
                                            }

                                        params.append(" -M ");
                                        params.append(infosys);

                                        bool ready = DBSingleton::instance().getDBObjectInstance()->isFileReadyStateV(fileIds);

                                        if (ready)
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                                                pr = new ExecuteProcess(cmd, params);
                                                if (pr)
                                                    {
                                                        /*check if fork failed , check if execvp failed, */
                                                        if (-1 == pr->executeProcessShell())
                                                            {
                                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to spawn " << commit;
                                                                DBSingleton::instance().getDBObjectInstance()->forkFailedRevertStateV(fileIds);
                                                            }
                                                        else
                                                            {
                                                                DBSingleton::instance().getDBObjectInstance()->setPidV(pr->getPid(), fileIds);
                                                                std::map<int, std::string>::const_iterator iterFileIds;
                                                                for (iterFileIds = fileIds.begin(); iterFileIds != fileIds.end(); ++iterFileIds)
                                                                    {
                                                                        struct message_updater msg;
                                                                        if(std::string(job_id).length() <= 37)
                                                                            {
                                                                                strcpy(msg.job_id, std::string(job_id).c_str());
                                                                                msg.file_id = iterFileIds->first;
                                                                                msg.process_id = (int) pr->getPid();
                                                                                msg.timestamp = milliseconds_since_epoch();
                                                                                ThreadSafeList::get_instance().push_back(msg);
                                                                            }
                                                                        else
                                                                            {
                                                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message length overun" << std::string(job_id).length() << commit;
                                                                            }
                                                                    }
                                                            }
                                                        delete pr;
                                                    }
                                            }
                                        params.clear();
                                    }

                                jobsReuse2.clear();

                                for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                    {
                                        if(*queueiter)
                                            delete *queueiter;
                                    }
                                voQueues[vo].clear();
                                fileIds.clear();

                            }
                    }
            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
            }
    }

    /* ---------------------------------------------------------------------- */

    void executeTransfer_a()
    {
        static bool drainMode = false;

        while (1)
            {
                try
                    {
                        if (stopThreads)
                            {
                                return;
                            }

                        if (DrainMode::getInstance())
                            {
                                if (!drainMode)
                                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more transfers for this instance!" << commit;
                                drainMode = true;
                                sleep(1);
                                continue;
                            }
                        else
                            {
                                drainMode = false;
                            }

                        //check for available resources
                        int currentActiveTransfers = DBSingleton::instance().getDBObjectInstance()->activeProcessesForThisHost();
                        if (maximumThreads != 0 && currentActiveTransfers != 0 && (currentActiveTransfers * 8) >= static_cast<int>(maximumThreads))
                            {
                                /*verify it's correct, you never know */
                                int countTr = proc_find(cmd.c_str());
                                if (countTr == -1)
                                    {
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to open proc fs" << commit;
                                    }
                                else if(countTr == 0)
                                    {
                                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is not right, active in db " <<  currentActiveTransfers << " and proc fs " << countTr << commit;
                                    }
                                else
                                    {
                                        if(countTr < currentActiveTransfers || (countTr * 8) <= static_cast<int>(maximumThreads))
                                            {
                                                /*do nothing*/
                                            }
                                        else
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Enforced soft limits, currently " << currentActiveTransfers << " are running" << commit;
                                                continue;
                                            }
                                    }
                            }

                        long unsigned int freeRam = getAvailableMemory();
                        if (freeRam != 0 && freeRam < 150)
                            {
                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Enforced limits, free RAM is " << freeRam << "MB and " << currentActiveTransfers << " are running" << commit;
                                continue;
                            }

                        /*check for non-reused jobs*/
                        executeUrlcopy(jobsReuse, false);


                        /* --- session reuse section ---*/
                        /*get jobs in submitted state and session reuse on*/
                        DBSingleton::instance().getDBObjectInstance()->getSubmittedJobsReuse(jobsReuse, allowedVOs);

                        if (!jobsReuse.empty())
                            {
                                executeUrlcopy(jobsReuse, true);
                                std::vector<TransferJobs*>::iterator iter2;
                                for (iter2 = jobsReuse.begin(); iter2 != jobsReuse.end(); ++iter2)
                                    {
                                        if(*iter2)
                                            delete *iter2;
                                    }
                                jobsReuse.clear();
                            }
                    }
                catch (std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler " << e.what() << commit;
                        if (!jobsReuse.empty())
                            {
                                std::vector<TransferJobs*>::iterator iter2;
                                for (iter2 = jobsReuse.begin(); iter2 != jobsReuse.end(); ++iter2)
                                    {
                                        if(*iter2)
                                            delete *iter2;
                                    }
                                jobsReuse.clear();
                            }
                        sleep(1);
                    }
                catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
                        if (!jobsReuse.empty())
                            {
                                std::vector<TransferJobs*>::iterator iter2;
                                for (iter2 = jobsReuse.begin(); iter2 != jobsReuse.end(); ++iter2)
                                    {
                                        if(*iter2)
                                            delete *iter2;
                                    }
                                jobsReuse.clear();
                            }
                        sleep(1);
                    }
                sleep(1);
            } /*end while*/
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

