/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ReuseTransfersService.h"

#include <glib.h>
#include <fstream>

#include "common/definitions.h"
#include "config/ServerConfig.h"
#include "cred/DelegCred.h"
#include "profiler/Macros.h"
#include "ConfigurationAssigner.h"
#include "ExecuteProcess.h"
#include "server/DrainMode.h"
#include "server/services/webservice/ws/SingleTrStateInstance.h"

#include "CloudStorageConfig.h"
#include "FileTransferScheduler.h"
#include "ThreadSafeList.h"


using namespace fts3::common;
using fts3::config::ServerConfig;


namespace fts3 {
namespace server {

extern time_t retrieveRecords;


ReuseTransfersService::ReuseTransfersService()
{
    setServiceName("ReuseTransfersService");
}


void ReuseTransfersService::runService()
{
    static bool drainMode = false;

    while (!boost::this_thread::interruption_requested())
    {
        retrieveRecords = time(0);

        try
        {
            if (DrainMode::instance())
            {
                if (!drainMode)
                    FTS3_COMMON_LOGGER_NEWLOG(INFO)
                            << "Set to drain mode, no more transfers for this instance!"
                            << commit;
                drainMode = true;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }
            else
            {
                drainMode = false;
            }

            executeUrlcopy();
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ReuseTransfersService " << e.what() << commit;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ReuseTransfersService!" << commit;
        }
        boost::this_thread::sleep(boost::posix_time::seconds(2));
    }
}


void ReuseTransfersService::writeJobFile(const std::string& jobId, const std::vector<std::string>& files)
{
    std::ofstream fout;
    try
    {
        std::vector<std::string>::const_iterator iter;
        std::string filename = ServerConfig::instance().get<std::string>("MessagingDirectory") + "/" + jobId;
        fout.open(filename.c_str(), std::ios::out);
        for (iter = files.begin(); iter != files.end(); ++iter)
        {
            fout << *iter << std::endl;
        }
    }
    catch (...)
    {
        // pass
    }
}


std::map<int, std::string> ReuseTransfersService::generateJobFile(
        const std::string& jobId, const std::list<TransferFile>& files)
{
    std::vector<std::string> urls;
    std::map<int, std::string> fileIds;
    std::ostringstream line;

    for (auto it = files.begin(); it != files.end(); ++it)
    {
        PROFILE_SCOPE("executeUrlcopy::for[reuse]");

        fileIds.insert(std::make_pair(it->fileId, it->jobId));

        std::string fileMetadata = UrlCopyCmd::prepareMetadataString(
                it->fileMetadata);
        if (fileMetadata.empty())
            fileMetadata = "x";

        std::string bringOnlineToken = it->bringOnlineToken;
        if (bringOnlineToken.empty())
            bringOnlineToken = "x";

        std::string checksum = it->checksum;
        if (checksum.empty())
            checksum = "x";

        line << std::fixed << it->fileId << " " << it->sourceSurl << " "
                << it->destSurl << " " << checksum << " "
                << boost::lexical_cast<long long>(it->userFilesize) << " "
                << fileMetadata << " " << bringOnlineToken;
        urls.push_back(line.str());
        line.str(std::string());
    }

    writeJobFile(jobId, urls);

    return fileIds;
}


void ReuseTransfersService::getFiles(const std::vector<QueueId>& queues)
{
    //now get files to be scheduled
    std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile> > > > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getReadySessionReuseTransfers(queues, voQueues);

    std::map<std::string,
            std::queue<std::pair<std::string, std::list<TransferFile> > > >::iterator vo_it;

    bool empty = false;

    while (!empty)
    {
        empty = true;
        for (vo_it = voQueues.begin(); vo_it != voQueues.end(); ++vo_it)
        {
            std::queue<std::pair<std::string, std::list<TransferFile> > > & vo_jobs =
                    vo_it->second;
            if (!vo_jobs.empty())
            {
                empty = false; //< if we are here there are still some data
                std::pair<std::string, std::list<TransferFile> > const job =
                        vo_jobs.front();
                vo_jobs.pop();
                startUrlCopy(job.first, job.second);
            }
        }
    }
}


void ReuseTransfersService::startUrlCopy(std::string const & job_id, std::list<TransferFile> const & files)
{
    GenericDbIfce *db = DBSingleton::instance().getDBObjectInstance();
    UrlCopyCmd cmd_builder;

    // Set log directory
    std::string logsDir = ServerConfig::instance().get<std::string>("TransferLogDirectory");;
    cmd_builder.setLogDir(logsDir);

    // Set parameters from the "representative", without using the source and destination url, and other data
    // that is per transfer
    TransferFile const & representative = files.front();
    cmd_builder.setFromTransfer(representative, true, db->getUserDnVisible());

    // Generate the file containing the list of transfers
    std::map<int, std::string> fileIds = generateJobFile(representative.jobId, files);

    /*check if manual config exist for this pair and vo*/
    std::vector<std::shared_ptr<ShareConfig> > cfgs;
    ConfigurationAssigner cfgAssigner(representative);
    cfgAssigner.assign(cfgs);

    FileTransferScheduler scheduler(representative, cfgs);
    int currentActive = 0;
    if (!scheduler.schedule(currentActive))
        return; /*SET TO READY STATE WHEN TRUE*/

    boost::optional<ProtocolResolver::protocol> user_protocol =
            ProtocolResolver::getUserDefinedProtocol(representative);

    if (user_protocol.is_initialized())
    {
        cmd_builder.setFromProtocol(user_protocol.get());
    }
    else
    {
        ProtocolResolver::protocol protocol;

        int level = db->getBufferOptimization();
        cmd_builder.setOptimizerLevel(level);
        if (level == 2)
        {
            protocol.nostreams = db->getStreamsOptimization(
                    representative.sourceSe, representative.destSe);
            if (protocol.nostreams == 0)
                protocol.nostreams = DEFAULT_NOSTREAMS;
        }
        else
        {
            protocol.nostreams = DEFAULT_NOSTREAMS;
        }

        protocol.urlcopy_tx_to = db->getGlobalTimeout();
        if (protocol.urlcopy_tx_to == 0)
        {
            protocol.urlcopy_tx_to = DEFAULT_TIMEOUT;
        }
        else
        {
            cmd_builder.setGlobalTimeout(protocol.urlcopy_tx_to);
        }
        int secPerMB = db->getSecPerMb();
        if (secPerMB > 0)
        {
            cmd_builder.setSecondsPerMB(secPerMB);
        }

        cmd_builder.setFromProtocol(protocol);
    }

    if (!cfgs.empty())
    {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: "
                << representative.sourceSe << " -> "
                << representative.destSe << commit;
        ProtocolResolver resolver(representative, cfgs);
        bool protocolExists = resolver.resolve();
        if (protocolExists)
        {
            ProtocolResolver::protocol protocol;

            protocol.nostreams = resolver.getNoStreams();
            protocol.tcp_buffer_size = resolver.getTcpBufferSize();
            protocol.urlcopy_tx_to = resolver.getUrlCopyTxTo();

            cmd_builder.setFromProtocol(protocol);
        }
    }

    std::string proxy_file = DelegCred::getProxyFile(representative.userDn, representative.credId);
    if (!proxy_file.empty())
        cmd_builder.setProxy(proxy_file);

    std::string cloudConfigFile = fts3::generateCloudStorageConfigFile(db, representative);
    if (!cloudConfigFile.empty()) {
        cmd_builder.setOAuthFile(cloudConfigFile);
    }

    // Send initial message
    SingleTrStateInstance::instance().sendStateMessage(job_id, -1);

    // Set all to ready, special case for session reuse
    int updatedFiles = db->updateFileStatusReuse(representative, "READY");
    if (updatedFiles <= 0)
    {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Transfer "
                << representative.jobId << " with session reuse enabled"
                << " not updated. Probably picked by another node"
                << commit;
        return;
    }

    // Debug level
    unsigned debugLevel = db->getDebugLevel(representative.sourceSe,
            representative.destSe);
    if (debugLevel > 0)
    {
        cmd_builder.setDebugLevel(debugLevel);
    }

    // Enable monitoring?
    cmd_builder.setMonitoring(monitoringMessages);

    // Infosystem
    cmd_builder.setInfosystem(infosys);

    // FTS3 name
    cmd_builder.setFTSName(ftsHostName);

    // Current number of actives
    cmd_builder.setNumberOfActive(currentActive);

    std::string params = cmd_builder.generateParameters();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd << " "
            << params << commit;
    ExecuteProcess pr(cmd, params);

    // Check if fork failed , check if execvp failed
    std::string forkMessage;
    if (-1 == pr.executeProcessShell(forkMessage))
    {
        if (forkMessage.empty())
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "Transfer failed to spawn " << commit;
            db->forkFailed(job_id);
        }
        else
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "Transfer failed to spawn " << forkMessage
                    << commit;
            db->forkFailed(job_id);
        }
    }
    else
    {
        db->setPidForJob(job_id, pr.getPid());
    }

    std::map<int, std::string>::const_iterator iterFileIds;
    for (iterFileIds = fileIds.begin(); iterFileIds != fileIds.end(); ++iterFileIds)
    {
        fts3::events::MessageUpdater msg2;
        msg2.set_job_id(job_id);
        msg2.set_file_id(iterFileIds->first);
        msg2.set_process_id(pr.getPid());
        msg2.set_timestamp(milliseconds_since_epoch());
        ThreadSafeList::get_instance().push_back(msg2);
    }
}


void ReuseTransfersService::executeUrlcopy()
{
    try
    {
        std::vector<QueueId> queues;
        DBSingleton::instance().getDBObjectInstance()->getQueuesWithSessionReusePending(queues);

        if (queues.empty()) {
            return;
        }

        getFiles(queues);
    }
    catch (std::exception& e)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ReuseTransfersService " << e.what() << commit;
    }
    catch (...)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in ReuseTransfersService!" << commit;
    }
}

} // end namespace server
} // end namespace fts3
