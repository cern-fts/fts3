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

#include <fstream>

#include "common/DaemonTools.h"
#include "config/ServerConfig.h"
#include "cred/DelegCred.h"
#include "ExecuteProcess.h"
#include "server/DrainMode.h"
#include "SingleTrStateInstance.h"

#include "CloudStorageConfig.h"
#include "ThreadSafeList.h"
#include "VoShares.h"


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
    while (!boost::this_thread::interruption_requested())
    {
        retrieveRecords = time(0);

        try
        {
            boost::this_thread::sleep(schedulingInterval);

            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                        << "Set to drain mode, no more transfers for this instance!"
                        << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            executeUrlcopy();
        }
        catch (boost::thread_interrupted&)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in ReuseTransfersService!" << commit;
            break;
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


std::map<uint64_t, std::string> ReuseTransfersService::generateJobFile(
        const std::string& jobId, const std::list<TransferFile>& files)
{
    std::vector<std::string> urls;
    std::map<uint64_t, std::string> fileIds;
    std::ostringstream line;

    for (auto it = files.begin(); it != files.end(); ++it)
    {
        fileIds.insert(std::make_pair(it->fileId, it->jobId));

        std::string fileMetadata = UrlCopyCmd::prepareMetadataString(
                it->fileMetadata);
        if (fileMetadata.empty())
            fileMetadata = "x";

        std::string archiveMetadata = UrlCopyCmd::prepareMetadataString(
                it->archiveMetadata);
        if (archiveMetadata.empty())
            archiveMetadata = "x";

        std::string bringOnlineToken = it->bringOnlineToken;
        if (bringOnlineToken.empty())
            bringOnlineToken = "x";

        std::string checksum = it->checksum;
        if (checksum.empty())
            checksum = "x";

        line << std::fixed
             << it->fileId << " "
             << it->sourceSurl << " "
             << it->destSurl << " "
             << checksum << " "
             << boost::lexical_cast<long long>(it->userFilesize) << " "
             << fileMetadata << " "
             << archiveMetadata << " "
             << bringOnlineToken << " "
             << it->scitag;
        urls.push_back(line.str());
        line.str(std::string());
    }

    writeJobFile(jobId, urls);

    return fileIds;
}


void ReuseTransfersService::getFiles(const std::vector<QueueId>& queues, int availableUrlCopySlots)
{
    auto db = DBSingleton::instance().getDBObjectInstance();

    //now get files to be scheduled
    std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile> > > > voQueues;
    db->getReadySessionReuseTransfers(queues, voQueues);

    std::map<std::string, int> slotsLeftForSource, slotsLeftForDestination;
    for (auto i = queues.begin(); i != queues.end(); ++i) {
        // To reduce queries, fill in one go limits as source and as destination
        if (slotsLeftForDestination.count(i->destSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->destSe);
            slotsLeftForDestination[i->destSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->destSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        if (slotsLeftForSource.count(i->sourceSe) == 0) {
            StorageConfig seConfig = db->getStorageConfig(i->sourceSe);
            slotsLeftForDestination[i->sourceSe] = seConfig.inboundMaxActive>0?seConfig.inboundMaxActive:60;
            slotsLeftForSource[i->sourceSe] = seConfig.outboundMaxActive>0?seConfig.outboundMaxActive:60;
        }
        // Once it is filled, decrement
        slotsLeftForDestination[i->destSe] -= i->activeCount;
        slotsLeftForSource[i->sourceSe] -= i->activeCount;
    }

    bool empty = false;

    while (!empty)
    {
        empty = true;
        for (auto vo_it = voQueues.begin(); vo_it != voQueues.end(); ++vo_it)
        {
            std::queue<std::pair<std::string, std::list<TransferFile> > > & vo_jobs =
                    vo_it->second;
            if (!vo_jobs.empty())
            {
                empty = false; //< if we are here there are still some data
                std::pair<std::string, std::list<TransferFile> > const job = vo_jobs.front();
                vo_jobs.pop();

                if (job.second.size() <= 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR)
                        << "Empty job " << job.first
                        << commit;
                    continue;
                }

                if (slotsLeftForDestination[job.second.front().destSe] <= 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                        << "Reached limitation for destination " << job.second.front().destSe
                        << commit;
                }
                else if (slotsLeftForSource[job.second.front().sourceSe] <= 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                        << "Reached limitation for source " << job.second.front().sourceSe
                        << commit;
                }
                else if (availableUrlCopySlots <= 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                    << "Reached limitation of MaxUrlCopyProcesses"
                    << commit;
                    return;
                } else {
                    startUrlCopy(job.first, job.second);
                    --availableUrlCopySlots;
                    --slotsLeftForDestination[job.second.front().destSe];
                    --slotsLeftForSource[job.second.front().sourceSe];
                }
            }
        }
    }
}


void ReuseTransfersService::startUrlCopy(std::string const & job_id, std::list<TransferFile> const & files)
{
    GenericDbIfce *db = DBSingleton::instance().getDBObjectInstance();
    UrlCopyCmd cmdBuilder;

    // Set log directory
    std::string logsDir = ServerConfig::instance().get<std::string>("TransferLogDirectory");
    cmdBuilder.setLogDir(logsDir);

    // Messaging
    std::string msgDir = ServerConfig::instance().get<std::string>("MessagingDirectory");
    cmdBuilder.setMonitoring(monitoringMessages, msgDir);

    // Set parameters from the "representative", without using the source and destination url, and other data
    // that is per transfer
    TransferFile const & representative = files.front();
    cmdBuilder.setFromTransfer(representative, true, db->publishUserDn(representative.voName), msgDir);

    // Generate the file containing the list of transfers
    std::map<uint64_t, std::string> fileIds = generateJobFile(representative.jobId, files);

    // Can we run?
    if (!db->isTrAllowed(representative.sourceSe, representative.destSe)) {
        return;
    }

    // Set parameters
    int secPerMB = db->getSecPerMb(representative.voName);
    if (secPerMB > 0) {
        cmdBuilder.setSecondsPerMB(secPerMB);
    }

    TransferFile::ProtocolParameters protocolParams = representative.getProtocolParameters();

    if (representative.internalFileParams.empty()) {
        protocolParams.nostreams = db->getStreamsOptimization(representative.sourceSe, representative.destSe);
        protocolParams.timeout = db->getGlobalTimeout(representative.voName);
        protocolParams.ipv6 = db->isProtocolIPv6(representative.sourceSe, representative.destSe);
        protocolParams.udt = db->isProtocolUDT(representative.sourceSe, representative.destSe);
        //protocolParams.buffersize
    }

    cmdBuilder.setFromProtocol(protocolParams);

    std::string proxy_file = DelegCred::getProxyFile(representative.userDn, representative.credId);
    if (!proxy_file.empty())
        cmdBuilder.setProxy(proxy_file);

    std::string cloudConfigFile = fts3::generateCloudStorageConfigFile(db, representative);
    if (!cloudConfigFile.empty()) {
        cmdBuilder.setOAuthFile(cloudConfigFile);
    }

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
    unsigned debugLevel = db->getDebugLevel(representative.sourceSe, representative.destSe);
    if (debugLevel > 0)
    {
        cmdBuilder.setDebugLevel(debugLevel);
    }

    // Infosystem
    cmdBuilder.setInfosystem(infosys);

    // FTS3 name
    cmdBuilder.setFTSName(ftsHostName);

    // Number of retries and maximum number allowed
    int retry_times = db->getRetryTimes(representative.jobId, representative.fileId);
    cmdBuilder.setNumberOfRetries(retry_times < 0 ? 0 : retry_times);

    int retry_max = db->getRetry(representative.jobId);
    cmdBuilder.setMaxNumberOfRetries(retry_max < 0 ? 0 : retry_max);

    // Log and run
    std::string params = cmdBuilder.generateParameters();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmdBuilder << commit;
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

    std::map<uint64_t, std::string>::const_iterator iterFileIds;
    std::vector<events::Message> protoMsgs;
    for (iterFileIds = fileIds.begin(); iterFileIds != fileIds.end(); ++iterFileIds)
    {
        fts3::events::MessageUpdater msg2;
        msg2.set_job_id(job_id);
        msg2.set_file_id(iterFileIds->first);
        msg2.set_process_id(pr.getPid());
        msg2.set_timestamp(millisecondsSinceEpoch());
        ThreadSafeList::get_instance().push_back(msg2);

        events::Message protoMsg;
        protoMsg.set_file_id(iterFileIds->first);
        protoMsg.set_transfer_status("UPDATE");
        protoMsg.set_timeout(cmdBuilder.getTimeout());
        protoMsg.set_buffersize(cmdBuilder.getBuffersize());
        protoMsg.set_nostreams(cmdBuilder.getNoStreams());
        protoMsgs.push_back(protoMsg);
    }

    // Set known protocol settings
    db::DBSingleton::instance().getDBObjectInstance()->updateProtocol(protoMsgs);
}


/**
 * Transfers in uneschedulable queues must be set to fail
 */
static void failUnschedulable(const std::vector<QueueId> &unschedulable)
{
    Producer producer(config::ServerConfig::instance().get<std::string>("MessagingDirectory"));

    std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile> > > > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getReadySessionReuseTransfers(unschedulable, voQueues);

    for (auto mapIter = voQueues.begin(); mapIter != voQueues.end(); ++mapIter) {
        std::queue<std::pair<std::string, std::list<TransferFile>>> &queues = mapIter->second;

        while (!queues.empty()) {
            std::pair<std::string, std::list<TransferFile>> &job = queues.front();
            for (auto iterTransfer = job.second.begin(); iterTransfer != job.second.end(); ++iterTransfer) {
                events::Message status;

                status.set_transfer_status("FAILED");
                status.set_timestamp(millisecondsSinceEpoch());
                status.set_process_id(0);
                status.set_job_id(job.first);
                status.set_file_id(iterTransfer->fileId);
                status.set_source_se(iterTransfer->sourceSe);
                status.set_dest_se(iterTransfer->destSe);
                status.set_transfer_message("No share configured for this VO");
                status.set_retry(false);
                status.set_errcode(EPERM);

                producer.runProducerStatus(status);
            }
            queues.pop();
        }
    }
}


void ReuseTransfersService::executeUrlcopy()
{
    // Bail out as soon as possible if there are too many url-copy processes
    int maxUrlCopy = config::ServerConfig::instance().get<int>("MaxUrlCopyProcesses");
    int urlCopyCount = countProcessesWithName("fts_url_copy");
    int availableUrlCopySlots = maxUrlCopy - urlCopyCount;

    if (availableUrlCopySlots <= 0) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING)
            << "Reached limitation of MaxUrlCopyProcesses"
            << commit;
        return;
    }

    try
    {
        std::vector<QueueId> queues, unschedulable;
        DBSingleton::instance().getDBObjectInstance()->getQueuesWithSessionReusePending(queues);
        // Breaking determinism. See FTS-704 for an explanation.
        std::random_shuffle(queues.begin(), queues.end());
        queues = applyVoShares(queues, unschedulable);
        // Fail all that are unschedulable
        failUnschedulable(unschedulable);

        if (queues.empty()) {
            return;
        }

        getFiles(queues, availableUrlCopySlots);
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
