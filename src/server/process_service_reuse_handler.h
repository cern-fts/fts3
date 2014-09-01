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

#include "process_service_handler.h"

extern bool stopThreads;
extern time_t retrieveRecords;


FTS3_SERVER_NAMESPACE_START
using namespace FTS3_COMMON_NAMESPACE;
using namespace db;
using namespace FTS3_CONFIG_NAMESPACE;

template
<
typename TRAITS
>
class ProcessServiceReuseHandler : public ProcessServiceHandler<TRAITS>
{

public:

    /* ---------------------------------------------------------------------- */

    typedef ProcessServiceReuseHandler <TRAITS> OwnType;

    /* ---------------------------------------------------------------------- */

    /** Constructor. */
    ProcessServiceReuseHandler(std::string const & desc = "") : ProcessServiceHandler<TRAITS>(desc) {}

    /* ---------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ProcessServiceReuseHandler() {}

    /* ---------------------------------------------------------------------- */

    void executeTransfer_p()
    {
        boost::function<void() > op = boost::bind(&ProcessServiceReuseHandler::executeTransfer_a, this);
        this->_enqueue(op);
    }

protected:

    std::vector<TransferJobs*> jobsReuse;

    using ProcessServiceHandler<TRAITS>::siteResolver;
    using ProcessServiceHandler<TRAITS>::ftsHostName;
    using ProcessServiceHandler<TRAITS>::allowedVOs;
    using ProcessServiceHandler<TRAITS>::infosys;
    using ProcessServiceHandler<TRAITS>::monitoringMessages;
    using ProcessServiceHandler<TRAITS>::execPoolSize;
    using ProcessServiceHandler<TRAITS>::cmd;
    using ProcessServiceHandler<TRAITS>::createJobFile;

    void executeUrlcopy(std::vector<TransferJobs*>& jobsReuse2)
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
                std::string oauth_file("");
                unsigned debugLevel = 0;

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
                bool StrictCopy = false;
                bool manualProtocol = false;
                std::string jobMetadata("");
                std::string fileMetadata("");
                std::string bringonlineToken("");
                bool userProtocol = false;
                std::string checksumMethod("");
                std::string userCred;

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
                        userCred = temp.USER_CREDENTIALS;

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
                        StrictCopy = (*p).strict_copy;
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

                        oauth_file = fts3::generateOauthConfigFile(DBSingleton::instance().getDBObjectInstance(), dn, userCred);

                        //send SUBMITTED message
                        SingleTrStateInstance::instance().sendStateMessage(tempUrl.JOB_ID, -1);


                        /*set all to ready, special case for session reuse*/
                        DBSingleton::instance().getDBObjectInstance()->updateFileStatusReuse(tempUrl, "READY");

                        for (queueiter = voQueues[vo].begin(); queueiter != voQueues[vo].end(); ++queueiter)
                            {
                                TransferFiles temp = *queueiter;
                                fileIds.insert(std::make_pair(temp.FILE_ID, temp.JOB_ID));
                            }

            /*these 3 must be the very first arguments, do not change, if more arguments needed just append in the end*/
            /*START*/
            params.append(" -a ");
                                params.append(job_id);

            if (multihop)
                                    params.append(" --multi-hop ");
                                else
                                    params.append(" -G ");
            /*END*/


                                debugLevel = DBSingleton::instance().getDBObjectInstance()->getDebugLevel(source_hostname, destin_hostname);
                                if (debugLevel)
                                    {
                                        params.append(" -debug=");
                                        params.append(boost::lexical_cast<std::string>(debugLevel));
                                        params.append(" ");
                                    }

                                if (StrictCopy)
                                    {
                                        params.append(" --strict-copy ");
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

                                if (userCred.length() > 0)
                                    {
                                        params.append(" -oauth ");
                                        params.append(oauth_file);
                                    }

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

                                params.append(" -7 ");
                                params.append(ftsHostName);

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
                                sleep(15);
                                continue;
                            }
                        else
                            {
                                drainMode = false;
                            }

                        /* --- session reuse section ---*/
                        /*get jobs in submitted state and session reuse on*/
                        DBSingleton::instance().getDBObjectInstance()->getSubmittedJobsReuse(jobsReuse, allowedVOs);

                        if (!jobsReuse.empty())
                            {
                                executeUrlcopy(jobsReuse);
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
                        sleep(2);
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
                        sleep(2);
                    }
                sleep(2);
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

