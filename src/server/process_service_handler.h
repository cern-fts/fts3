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
#include "common/ExecutorPool.h"
#include "process.h"
#include <iostream>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <sstream>
#include "site_name.h"
#include "FileTransferScheduler.h"
#include "FileTransferExecutor.h"
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
#include <boost/scoped_ptr.hpp>

extern bool stopThreads;
extern time_t retrieveRecords;


FTS3_SERVER_NAMESPACE_START
using FTS3_COMMON_NAMESPACE::Pointer;
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;



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

        execPoolSize = theServerConfig().get<int> ("InternalThreadPool");

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
    std::string infosys;
    bool monitoringMessages;
    int execPoolSize;
    std::string cmd;

    std::string extractHostname(const std::string &surl)
    {
        Uri u0 = Uri::Parse(surl);
        return u0.Protocol + "://" + u0.Host;
    }

    void createJobFile(std::string job_id, std::vector<std::string>& files)
    {
        std::ofstream fout;
        try
            {
                std::vector<std::string>::const_iterator iter;
                std::string filename = "/var/lib/fts3/" + job_id;
                fout.open(filename.c_str(), ios::out);
                for (iter = files.begin(); iter != files.end(); ++iter)
                    {
                        fout << *iter << std::endl;
                    }
                fout.close();
            }
        catch(...)
            {
                fout.close();
            }
    }

    void getFiles( std::vector< boost::tuple<std::string, std::string, std::string> >& distinct, std::map< std::string, std::list<TransferFiles> >& voQueues)
    {
        try
            {
                if(distinct.empty())
                    return;

                //now get files to be scheduled
                DBSingleton::instance().getDBObjectInstance()->getByJobId(distinct, voQueues);

                if(voQueues.empty())
                    return;

                // create transfer-file handler
                TransferFileHandler tfh(voQueues);

                // the worker thread pool
                ExecutorPool<FileTransferExecutor> execPool(execPoolSize);

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
                                        execPool.stop();
                                        return;
                                    }

                                TransferFiles tf = tfh.get(*it_vo);


                                FileTransferExecutor* exec = new FileTransferExecutor(
                                    tf,
                                    tfh,
                                    monitoringMessages,
                                    infosys,
                                    ftsHostName
                                );

                                execPool.add(exec);

                            }
                    }

                // wait for all the workers to finish
                execPool.join();

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Threadpool processed: " << initial_size << " files (" << execPool.executed() << " have been scheduled)" << commit;

            }
        catch (std::exception& e)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler:getFiles " << e.what() << commit;
            }
        catch (...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_handler!" << commit;
            }
    }

    void executeUrlcopy(std::vector<TransferJobs*>& jobsReuse2, bool reuse)
    {
        try
            {
                std::string params = std::string("");
                std::string sourceSiteName("");
                std::string destSiteName("");
                std::string source_hostname("");
                std::string destin_hostname("");
                SeProtocolConfig protocol;
                std::string proxy_file("");
                bool debug = false;

                if (reuse == false)
                    {

                        //get distinct source, dest, vo first
                        std::vector< boost::tuple<std::string, std::string, std::string> > distinct;
                        std::map< std::string, std::list<TransferFiles> > voQueues1;
                        std::map< std::string, std::list<TransferFiles> > voQueues2;
                        std::map< std::string, std::list<TransferFiles> > voQueues3;
                        std::map< std::string, std::list<TransferFiles> > voQueues4;
                        std::map< std::string, std::list<TransferFiles> > voQueues; //merged

                        boost::thread_group g;

                        DBSingleton::instance().getDBObjectInstance()->getVOPairs(distinct);

                        if(distinct.empty())
                            return;

                        std::size_t const half_size1 = distinct.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_1(distinct.begin(), distinct.begin() + half_size1);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_2(distinct.begin() + half_size1, distinct.end());

                        std::size_t const half_size2 = split_1.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_11(split_1.begin(), split_1.begin() + half_size2);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_21(split_1.begin() + half_size2, split_1.end());

                        std::size_t const half_size3 = split_2.size() / 2;
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_12(split_2.begin(), split_2.begin() + half_size3);
                        std::vector< boost::tuple<std::string, std::string, std::string> > split_22(split_2.begin() + half_size3, split_2.end());

                        g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_11), boost::ref(voQueues1)));
                        g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_21), boost::ref(voQueues2)));
                        g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_12), boost::ref(voQueues3)));
                        g.create_thread(boost::bind(&ProcessServiceHandler::getFiles, this, boost::ref(split_22), boost::ref(voQueues4)));

                        // wait for them
                        g.join_all();

                        voQueues1.clear();
                        voQueues2.clear();
                        voQueues3.clear();
                        voQueues4.clear();
                        voQueues.clear();
                        distinct.clear();
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
                                bool manualProtocol = false;
                                std::string jobMetadata("");
                                std::string fileMetadata("");
                                std::string bringonlineToken("");
                                bool userProtocol = false;
                                std::string checksumMethod("");

                                TransferFiles tempUrl;

                                std::map< std::string, std::list<TransferFiles> > voQueues;
                                std::list<TransferFiles>::const_iterator queueiter;

                                DBSingleton::instance().getDBObjectInstance()->getByJobIdReuse(jobsReuse2, voQueues);

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
                                bool multihop = (jobsReuse2.front()->REUSE == "H");

                                for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                    {
                                        PROFILE_SCOPE("executeUrlcopy::for[reuse]");
                                        if (stopThreads)
                                            {
                                                return;
                                            }

                                        TransferFiles temp = *queueiter;
                                        tempUrl = temp;
                                        surl = temp.SOURCE_SURL;
                                        durl = temp.DEST_SURL;
                                        job_id = temp.JOB_ID;
                                        vo_name = temp.VO_NAME;
                                        cred_id = temp.CRED_ID;
                                        dn = temp.DN;
                                        file_id = temp.FILE_ID;
                                        overwrite = temp.OVERWRITE;
                                        source_hostname = temp.SOURCE_SE;
                                        destin_hostname = temp.DEST_SE;
                                        source_space_token = temp.SOURCE_SPACE_TOKEN;
                                        dest_space_token = temp.DEST_SPACE_TOKEN;
                                        pinLifetime = temp.PIN_LIFETIME;
                                        bringOnline = temp.BRINGONLINE;
                                        userFilesize = temp.USER_FILESIZE;
                                        jobMetadata = prepareMetadataString(temp.JOB_METADATA);
                                        fileMetadata = prepareMetadataString(temp.FILE_METADATA);
                                        bringonlineToken = temp.BRINGONLINE_TOKEN;
                                        checksumMethod = temp.CHECKSUM_METHOD;

                                        if (fileMetadata.length() <= 0)
                                            fileMetadata = "x";
                                        if (bringonlineToken.length() <= 0)
                                            bringonlineToken = "x";
                                        if (std::string(temp.CHECKSUM_METHOD).length() > 0)
                                            {
                                                if (std::string(temp.CHECKSUM).length() > 0)
                                                    checksum = temp.CHECKSUM;
                                                else
                                                    checksum = "x";
                                            }
                                        else
                                            {
                                                checksum = "x";
                                            }

                                        url << std::fixed << file_id << " " << surl << " " << durl << " " << checksum << " " << boost::lexical_cast<long long>(userFilesize) << " " << fileMetadata << " " << bringonlineToken;
                                        urls.push_back(url.str());
                                        url.str("");
                                    }


                                //disable for now, remove later
                                sourceSiteName = ""; //siteResolver.getSiteName(surl);
                                destSiteName = ""; //siteResolver.getSiteName(durl);

                                createJobFile(job_id, urls);

                                /*check if manual config exist for this pair and vo*/
                                vector< boost::shared_ptr<ShareConfig> > cfgs;
                                ConfigurationAssigner cfgAssigner(tempUrl);
                                cfgAssigner.assign(cfgs);

                                optional<ProtocolResolver::protocol> p = ProtocolResolver::getUserDefinedProtocol(tempUrl);

                                if (p.is_initialized())
                                    {
                                        BufSize = (*p).tcp_buffer_size;
                                        StreamsperFile = (*p).nostreams;
                                        Timeout = (*p).urlcopy_tx_to;
                                        manualProtocol = true;
                                    }
                                else
                                    {
                                        BufSize = DBSingleton::instance().getDBObjectInstance()->getBufferOptimization();
                                        StreamsperFile = DBSingleton::instance().getDBObjectInstance()->getStreamsOptimization(source_hostname, destin_hostname);
                                        Timeout = DBSingleton::instance().getDBObjectInstance()->getGlobalTimeout();
                                        if(Timeout == 0)
                                            Timeout = DEFAULT_TIMEOUT;
                                        else
                                            params.append(" -Z ");

                                        int secPerMB = DBSingleton::instance().getDBObjectInstance()->getSecPerMb();
                                        if(secPerMB > 0)
                                            {
                                                params.append(" -V ");
                                                params.append(lexical_cast<string >(secPerMB));
                                            }
                                    }

                                FileTransferScheduler scheduler(tempUrl, cfgs);
                                if (scheduler.schedule())   /*SET TO READY STATE WHEN TRUE*/
                                    {
                                        bool isAutoTuned = false;
                                        std::stringstream internalParams;

                                        if (!cfgs.empty())
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " << destin_hostname << commit;
                                                ProtocolResolver resolver(tempUrl, cfgs);
                                                bool protocolExists = resolver.resolve();
                                                if (protocolExists)
                                                    {
                                                        manualConfigExists = true;
                                                        protocol.NOSTREAMS = resolver.getNoStreams();
                                                        protocol.NO_TX_ACTIVITY_TO = resolver.getNoTxActiveTo();
                                                        protocol.TCP_BUFFER_SIZE = resolver.getTcpBufferSize();
                                                        protocol.URLCOPY_TX_TO = resolver.getUrlCopyTxTo();
                                                    }

                                                if (resolver.isAuto())
                                                    {
                                                        isAutoTuned = true;
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
                                        DBSingleton::instance().getDBObjectInstance()->updateFileStatusReuse(tempUrl, "READY");

                                        for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                                            {
                                                TransferFiles temp = *queueiter;
                                                fileIds.insert(std::make_pair(temp.FILE_ID, temp.JOB_ID));
                                            }

                                        SingleTrStateInstance::instance().sendStateMessage(tempUrl.JOB_ID, -1);


                                        debug = DBSingleton::instance().getDBObjectInstance()->getDebugMode(source_hostname, destin_hostname);
                                        if (debug == true)
                                            {
                                                params.append(" -F ");
                                            }

                                        if (manualConfigExists || userProtocol)
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

                                        if (multihop)
                                            params.append(" --multi-hop ");
                                        else
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





                                        if (!manualConfigExists)
                                            {
                                                params.append(" -e ");
                                                params.append(lexical_cast<string >(StreamsperFile));
                                            }
                                        else
                                            {
                                                if (protocol.NOSTREAMS >= 0)
                                                    {
                                                        params.append(" -e ");
                                                        params.append(lexical_cast<string >(protocol.NOSTREAMS));
                                                    }
                                                else
                                                    {
                                                        params.append(" -e ");
                                                        params.append(lexical_cast<string >(DEFAULT_NOSTREAMS));
                                                    }
                                            }

                                        if (!manualConfigExists)
                                            {
                                                params.append(" -f ");
                                                params.append(lexical_cast<string >(BufSize));
                                            }
                                        else
                                            {
                                                if (protocol.TCP_BUFFER_SIZE >= 0)
                                                    {
                                                        params.append(" -f ");
                                                        params.append(lexical_cast<string >(protocol.TCP_BUFFER_SIZE));
                                                    }
                                                else
                                                    {
                                                        params.append(" -f ");
                                                        params.append(lexical_cast<string >(DEFAULT_BUFFSIZE));
                                                    }
                                            }

                                        if (!manualConfigExists)
                                            {
                                                params.append(" -h ");
                                                params.append(lexical_cast<string >(Timeout));
                                            }
                                        else
                                            {
                                                if (protocol.URLCOPY_TX_TO >= 0)
                                                    {
                                                        params.append(" -h ");
                                                        params.append(lexical_cast<string >(protocol.URLCOPY_TX_TO));
                                                    }
                                                else
                                                    {
                                                        params.append(" -h ");
                                                        params.append(lexical_cast<string >(DEFAULT_TIMEOUT));
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

                                        if (std::string(checksumMethod).length() > 0)
                                            {
                                                params.append(" -A ");
                                                params.append(checksumMethod);
                                            }

                                        params.append(" -M ");
                                        params.append(infosys);

                                        params.append(" -Y ");
                                        params.append(prepareMetadataString(dn));


                                        bool ready = DBSingleton::instance().getDBObjectInstance()->isFileReadyStateV(fileIds);

                                        if (ready)
                                            {
                                                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " " << params << commit;
                                                ExecuteProcess pr(cmd, params);
                                                /*check if fork failed , check if execvp failed, */
                                                std::string forkMessage;
                                                if (-1 == pr.executeProcessShell(forkMessage))
                                                    {
                                                        if(forkMessage.empty())
                                                            {
                                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to spawn " << commit;
                                                                DBSingleton::instance().getDBObjectInstance()->forkFailedRevertStateV(fileIds);
                                                            }
                                                        else
                                                            {
                                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to spawn " << forkMessage << commit;
                                                                DBSingleton::instance().getDBObjectInstance()->forkFailedRevertStateV(fileIds);
                                                            }
                                                    }
                                                else
                                                    {
                                                        DBSingleton::instance().getDBObjectInstance()->setPidV(pr.getPid(), fileIds);
                                                    }
                                                std::map<int, std::string>::const_iterator iterFileIds;
                                                for (iterFileIds = fileIds.begin(); iterFileIds != fileIds.end(); ++iterFileIds)
                                                    {
                                                        struct message_updater msg2;
                                                        if(std::string(job_id).length() <= 37)
                                                            {
                                                                strncpy(msg2.job_id, std::string(job_id).c_str(), sizeof(msg2.job_id));
                                                                msg2.job_id[sizeof(msg2.job_id) - 1] = '\0';
                                                                msg2.file_id = iterFileIds->first;
                                                                msg2.process_id = (int) pr.getPid();
                                                                msg2.timestamp = milliseconds_since_epoch();
                                                                ThreadSafeList::get_instance().push_back(msg2);
                                                            }
                                                        else
                                                            {
                                                                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Message length overun" << std::string(job_id).length() << commit;
                                                            }
                                                    }
                                            }
                                        params.clear();
                                    }

                                jobsReuse2.clear();
                                voQueues.clear();
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
        static int reuseExec = 0;

        while (1)
            {
                retrieveRecords = time(0);

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
                                sleep(3);
                                continue;
                            }
                        else
                            {
                                drainMode = false;
                            }

                        /*check for non-reused jobs*/
                        executeUrlcopy(jobsReuse, false);

                        if (stopThreads)
                            return;


                        /* --- session reuse section ---*/
                        /*get jobs in submitted state and session reuse on*/
                        if(++reuseExec == 2)
                            {
                                DBSingleton::instance().getDBObjectInstance()->getSubmittedJobsReuse(jobsReuse, allowedVOs);
                                reuseExec = 0;
                            }

                        if (stopThreads)
                            return;

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
                        reuseExec = 0;
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
                        sleep(3);
                    }
                catch (...)
                    {
                        reuseExec = 0;
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
                        sleep(3);
                    }
                sleep(3);
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

