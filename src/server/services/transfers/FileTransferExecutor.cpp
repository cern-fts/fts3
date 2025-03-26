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

#include "FileTransferExecutor.h"

#include "common/Logger.h"
#include "ExecuteProcess.h"
#include "SingleTrStateInstance.h"

#include "config/ServerConfig.h"
#include "CloudStorageConfig.h"
#include "ThreadSafeList.h"
#include "UrlCopyCmd.h"
#include <iostream>

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

namespace fts3
{

namespace server
{


FileTransferExecutor::FileTransferExecutor(TransferFile &tf,
    bool monitoringMsg, std::string infosys,
    std::string ftsHostName, std::string proxy, std::string logDir, std::string msgDir) :
    tf(tf),
    monitoringMsg(monitoringMsg),
    infosys(infosys),
    ftsHostName(ftsHostName),
    proxy(proxy),
    logsDir(logDir),
    msgDir(msgDir),
    db(DBSingleton::instance().getDBObjectInstance())
{
}


FileTransferExecutor::~FileTransferExecutor()
{

}

std::string FileTransferExecutor::getAuthMethod(const std::string& jobMetadata)
{
    if (jobMetadata != "null") {
        std::stringstream iostr(jobMetadata);
        pt::ptree job_metadata;

        try {
            pt::read_json(iostr, job_metadata);
            return job_metadata.get<std::string>("auth_method", "null");
        } catch (...) {
            return "null";
        }
    }

    return "null";
}

void FileTransferExecutor::run(boost::any & ctx)
{
    if (ctx.empty()) {
        ctx = 0;
    }

    int &scheduled = boost::any_cast<int &>(ctx);

    //stop forking when a signal is received to avoid deadlocks
    if (tf.fileId == 0 || boost::this_thread::interruption_requested()) {
        return;
    }

    try {
        // if the pair was already checked and not scheduled skip it
        if (notScheduled.count(make_pair(tf.sourceSe, tf.destSe))) {
            return;
        }

        // check if manual config exist for this pair and vo
        std::vector< std::shared_ptr<ShareConfig> > cfgs;

        // Set to READY state when true
        if (db->isTrAllowed(tf.sourceSe, tf.destSe))
        {
            UrlCopyCmd cmdBuilder;

            int secPerMB = db->getSecPerMb(tf.voName);
            if (secPerMB > 0) {
                cmdBuilder.setSecondsPerMB(secPerMB);
            }

            TransferFile::ProtocolParameters protocolParams = tf.getProtocolParameters();

            if (tf.internalFileParams.empty()) {
                protocolParams.nostreams = db->getStreamsOptimization(tf.sourceSe, tf.destSe);
                protocolParams.timeout = db->getGlobalTimeout(tf.voName);
                protocolParams.ipv6 = db->isProtocolIPv6(tf.sourceSe, tf.destSe);
                protocolParams.udt = db->isProtocolUDT(tf.sourceSe, tf.destSe);
                //protocolParams.buffersize
            }

            cmdBuilder.setFromProtocol(protocolParams);

            // Update from the transfer
            cmdBuilder.setFromTransfer(tf, false, db->publishUserDn(tf.voName), msgDir);

            // Set Auth method in the command line options
            std::string authMethod = FileTransferExecutor::getAuthMethod(tf.jobMetadata);
            cmdBuilder.setAuthMethod(authMethod);

            // Cloud storage credentials
            std::string cloudStorageConfig = generateCloudStorageConfigFile(db, tf, authMethod);
            if (!cloudStorageConfig.empty()) {
                cmdBuilder.setCloudConfig(cloudStorageConfig);
            }

            // Not a cloud storage transfer but still using oauth2 method
            if (cloudStorageConfig.empty() && "oauth2" == authMethod) {
                std::string oauthCredentials = generateOAuthConfigFile(db, tf);
                if (!oauthCredentials.empty()) {
                    cmdBuilder.setOAuthFile(oauthCredentials);
                    // Should be set via the "setFromTransfer()" function, but ATs are not
                    // Keep these functions grouped together until refactoring
                    cmdBuilder.setSourceTokenId(tf.sourceTokenId);
                    cmdBuilder.setDestinationTokenId(tf.destinationTokenId);
                    cmdBuilder.setTokenRefreshMarginPeriod(fts3::config::ServerConfig::instance().get<int>("TokenRefreshMarginPeriod"));
                }
            }

            // Debug level
            cmdBuilder.setDebugLevel(db->getDebugLevel(tf.sourceSe, tf.destSe));

            // Disable delegation (according to link config)
            cmdBuilder.setDisableDelegation(db->getDisableDelegationFlag(tf.sourceSe, tf.destSe));

            // Get SRM 3rd party TURL (according to link config)
            auto thirdPartyTURL = db->getThirdPartyTURL(tf.sourceSe, tf.destSe);
            if (!thirdPartyTURL.empty()) {
                cmdBuilder.setThirdPartyTURL(thirdPartyTURL);
            }

            // Disable streaming via local transfers (according to global config)
            cmdBuilder.setDisableStreaming(db->getDisableStreamingFlag(tf.voName));

            // Enable monitoring
            cmdBuilder.setMonitoring(monitoringMsg, msgDir);

            // Set UrlCopyProcess ping interval (in seconds)
            cmdBuilder.setPingInterval(fts3::config::ServerConfig::instance().get<int>("UrlCopyProcessPingInterval"));

            // Set proxy path if authentication method is not OAuth2
            if (!proxy.empty() && authMethod != "oauth2") {
                cmdBuilder.setProxy(proxy);
            }

            // Info system
            if (!infosys.empty()) {
                cmdBuilder.setInfosystem(infosys);
            }

            // UDT and IPv6
            cmdBuilder.setUDT(db->isProtocolUDT(tf.sourceSe, tf.destSe));
            if (!cmdBuilder.isIPv6Explicit()) {
                cmdBuilder.setIPv6(db->isProtocolIPv6(tf.sourceSe, tf.destSe));
            }

            // Disable source file eviction from disk buffer (according to SE config)
            cmdBuilder.setSkipEvict(db->getSkipEvictionFlag(tf.sourceSe));

            // Set TPC mode (according to SE config)
            cmdBuilder.setCopyMode(db->getCopyMode(tf.sourceSe, tf.destSe));

            // FTS3 host name
            cmdBuilder.setFTSName(ftsHostName);

            // Number of retries and maximum number allowed
            int retry_times = db->getRetryTimes(tf.jobId, tf.fileId);
            cmdBuilder.setNumberOfRetries(retry_times < 0 ? 0 : retry_times);

            if ((retry_times > 0) && (tf.overwriteFlag == "R")) {
                cmdBuilder.setOverwrite(true);
            }

            // If is multihop job, file is not the final destination and overwrite requested => enable overwrite
            //   - overwriteFlag = "M" (overwrite-hop)
            //   - overwriteFlag = "Q" (overwrite-hop + overwrite-when-only-on-disk)
            if (tf.jobType == Job::kTypeMultiHop && !tf.lastHop &&
                (tf.overwriteFlag == "M" || tf.overwriteFlag == "Q")) {
                cmdBuilder.setOverwrite(true);
            }

            // If archiving job and overwrite-when-only-on-disk requested => enable overwrite on disk
            //   - overwriteFlag = "D"
            //   - multihop job, last hop and overwriteFlag = "Q"
            if (tf.archiveTimeout > 0) {
                bool overwriteOnDiskRequested =
                        (tf.overwriteFlag == "D") ||
                        (tf.jobType == Job::kTypeMultiHop && tf.lastHop && tf.overwriteFlag == "Q");

                if (overwriteOnDiskRequested) {
                    cmdBuilder.setOverwriteOnDisk(true);
                    // Also send "overwrite-disk-enabled" flag (decision delegated to the UrlCopyProcess)
                    cmdBuilder.setOverwriteDiskEnabled(db->getOverwriteDiskEnabledFlag(tf.destSe));
                }
            }

            int retry_max = db->getRetry(tf.jobId);
            cmdBuilder.setMaxNumberOfRetries(retry_max < 0 ? 0 : retry_max);

            // Log directory
            cmdBuilder.setLogDir(logsDir);

            // Build the parameters
            std::string params = cmdBuilder.generateParameters();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmdBuilder << commit;
            ExecuteProcess pr(UrlCopyCmd::Program, params);

            // check again here if the server has stopped - just in case
            if(boost::this_thread::interruption_requested()) {
                return;
            }

            scheduled += 1;

            boost::tuple<bool, std::string> fileUpdated = db->updateTransferStatus(
                    tf.jobId, tf.fileId, 0, "READY", "",
                    0, 0, 0.0, false, "");
            db->updateJobStatus(tf.jobId, "ACTIVE");

            // If fileUpdated == false, the transfer was *not* updated, which means we got
            // probably a collision with some other node
            if (!fileUpdated.get<0>()) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                    << "Transfer " << tf.jobId << " " << tf.fileId
                    << " not updated. Probably picked by another node" << commit;
                return;
            }

            // Update protocol parameters (specially interested on nostreams)
            events::Message protoMsg;
            protoMsg.set_transfer_status("UPDATE");
            protoMsg.set_file_id(tf.fileId);
            protoMsg.set_buffersize(cmdBuilder.getBuffersize());
            protoMsg.set_nostreams(cmdBuilder.getNoStreams());
            protoMsg.set_timeout(cmdBuilder.getTimeout());
            db->updateProtocol(std::vector<events::Message>{protoMsg});

            // Spawn the fts_url_copy
            bool failed = false;
            std::string forkMessage;
            if (-1 == pr.executeProcessShell(forkMessage)) {
                failed = true;
                db->updateTransferStatus(tf.jobId, tf.fileId, pr.getPid(),
                                         "FAILED", "Transfer failed to fork, check fts3server.log for more details",
                                         0, 0, 0.0, false, "");
                db->updateJobStatus(tf.jobId, "FAILED");

                if (forkMessage.empty()) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to fork "
                        << tf.jobId << "  " << tf.fileId << commit;
                }
                else {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Transfer failed to fork " << forkMessage << "   " << tf.jobId <<
                        "  " << tf.fileId << commit;
                }
            }
            else {
                db->updateTransferStatus(tf.jobId, tf.fileId, pr.getPid(), "READY", "",
                                         0, 0, 0.0, false, "");
            }

            // Send current state
            SingleTrStateInstance::instance().sendStateMessage(tf.jobId, tf.fileId);
            fts3::events::MessageUpdater msg;
            msg.set_job_id(tf.jobId);
            msg.set_file_id(tf.fileId);
            msg.set_process_id(pr.getPid());
            msg.set_timestamp(millisecondsSinceEpoch());

            // Only set watcher when the file has started
            if(!failed) {
                ThreadSafeList::get_instance().push_back(msg);
            }
        }
        else {
            notScheduled.insert(make_pair(tf.sourceSe, tf.destSe));
        }
    }
    catch (std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception " << e.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Process thread exception unknown" << commit;
    }
}

} /* namespace server */
} /* namespace fts3 */
