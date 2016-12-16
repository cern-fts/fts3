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

#include <glib.h>

#include "common/definitions.h"
#include "common/Logger.h"
#include "common/Uri.h"
#include "cred/CredUtility.h"
#include "cred/DelegCred.h"
#include "ConfigurationAssigner.h"
#include "ExecuteProcess.h"
#include "FileTransferScheduler.h"
#include "ProtocolResolver.h"
#include "SingleTrStateInstance.h"

#include "CloudStorageConfig.h"
#include "ThreadSafeList.h"
#include "UrlCopyCmd.h"


namespace fts3
{

namespace server
{


FileTransferExecutor::FileTransferExecutor(TransferFile &tf,
    TransferFileHandler &tfh, bool monitoringMsg, std::string infosys,
    std::string ftsHostName, std::string proxy, std::string logDir) :
    tf(tf),
    tfh(tfh),
    monitoringMsg(monitoringMsg),
    infosys(infosys),
    ftsHostName(ftsHostName),
    proxy(proxy),
    logsDir(logDir),
    db(DBSingleton::instance().getDBObjectInstance())
{
}


FileTransferExecutor::~FileTransferExecutor()
{

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
        std::string source_hostname = tf.sourceSe;
        std::string destin_hostname = tf.destSe;
        std::string params;

        // if the pair was already checked and not scheduled skip it
        if (notScheduled.count(make_pair(source_hostname, destin_hostname))) {
            return;
        }

        // check if manual config exist for this pair and vo

        std::vector< std::shared_ptr<ShareConfig> > cfgs;

        ConfigurationAssigner cfgAssigner(tf);
        cfgAssigner.assign(cfgs);

        FileTransferScheduler scheduler(
            tf,
            cfgs,
            tfh.getDestinations(source_hostname),
            tfh.getSources(destin_hostname),
            tfh.getDestinationsVos(source_hostname),
            tfh.getSourcesVos(destin_hostname)
        );

        int currentActive = 0;
        // Set to READY state when true
        if (scheduler.schedule(currentActive))
        {
            SeProtocolConfig protocol;
            UrlCopyCmd cmd_builder;

            boost::optional<ProtocolResolver::protocol> user_protocol = ProtocolResolver::getUserDefinedProtocol(tf);

            if (user_protocol.is_initialized()) {
                cmd_builder.setFromProtocol(user_protocol.get());
            }
            else {
                ProtocolResolver::protocol protocol;

                protocol.nostreams = db->getStreamsOptimization(tf.voName, source_hostname, destin_hostname);
                protocol.urlcopy_tx_to = db->getGlobalTimeout(tf.voName);
                if (protocol.urlcopy_tx_to > 0) {
                    cmd_builder.setGlobalTimeout(protocol.urlcopy_tx_to);
                }

                int secPerMB = db->getSecPerMb(tf.voName);
                if (secPerMB > 0) {
                    cmd_builder.setSecondsPerMB(secPerMB);
                }

                cmd_builder.setFromProtocol(protocol);
            }

            if (!cfgs.empty()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Check link config for: " << source_hostname << " -> " <<
                destin_hostname << commit;
                ProtocolResolver resolver(tf, cfgs);
                bool protocolExists = resolver.resolve();
                if (protocolExists) {
                    ProtocolResolver::protocol protocol;
                    protocol.nostreams = resolver.getNoStreams();
                    protocol.tcp_buffer_size = resolver.getTcpBufferSize();
                    protocol.urlcopy_tx_to = resolver.getUrlCopyTxTo();
                    protocol.strict_copy = resolver.getStrictCopy();
                    protocol.ipv6 = resolver.getIPv6();
                    cmd_builder.setFromProtocol(protocol);
                }
            }

            // Update from the transfer
            cmd_builder.setFromTransfer(tf, false, db->publishUserDn(tf.voName));

            // OAuth credentials
            std::string cloudConfigFile = generateCloudStorageConfigFile(db, tf);
            if (!cloudConfigFile.empty()) {
                cmd_builder.setOAuthFile(cloudConfigFile);
            }

            // Debug level
            cmd_builder.setDebugLevel(db->getDebugLevel(source_hostname, destin_hostname));

            // Enable monitoring
            cmd_builder.setMonitoring(monitoringMsg);

            // Proxy
            if (!proxy.empty()) {
                cmd_builder.setProxy(proxy);
            }

            // Info system
            if (!infosys.empty()) {
                cmd_builder.setInfosystem(infosys);
            }

            // UDT and IPv6
            cmd_builder.setUDT(db->isProtocolUDT(source_hostname, destin_hostname));
            if (!cmd_builder.isIPv6Explicit()) {
                cmd_builder.setIPv6(db->isProtocolIPv6(source_hostname, destin_hostname));
            }

            // FTS3 host name
            cmd_builder.setFTSName(ftsHostName);

            // Pass the number of active transfers for this link to url_copy
            cmd_builder.setNumberOfActive(currentActive);

            // Number of retries and maximum number allowed
            int retry_times = db->getRetryTimes(tf.jobId, tf.fileId);
            cmd_builder.setNumberOfRetries(retry_times < 0 ? 0 : retry_times);

            int retry_max = db->getRetry(tf.jobId);
            cmd_builder.setMaxNumberOfRetries(retry_max < 0 ? 0 : retry_max);

            // Log directory
            cmd_builder.setLogDir(logsDir);

            // Build the parameters
            std::string params = cmd_builder.generateParameters();
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Transfer params: " << cmd_builder << commit;
            ExecuteProcess pr(UrlCopyCmd::Program, params);

            // check again here if the server has stopped - just in case
            if(boost::this_thread::interruption_requested()) {
                return;
            }

            scheduled += 1;

            bool fileUpdated;
            fileUpdated = db->updateTransferStatus(
                tf.jobId, tf.fileId, 0.0, "READY", "",
                0, 0, 0, false
            );
            db->updateJobStatus(tf.jobId, "ACTIVE",0);

            // If fileUpdated == false, the transfer was *not* updated, which means we got
            // probably a collision with some other node
            if (!fileUpdated) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                    << "Transfer " << tf.jobId << " " << tf.fileId
                    << " not updated. Probably picked by another node" << commit;
                return;
            }

            // Update protocol parameters (specially interested on nostreams)
            events::Message protoMsg;
            protoMsg.set_transfer_status("UPDATE");
            protoMsg.set_file_id(tf.fileId);
            protoMsg.set_buffersize(cmd_builder.getBuffersize());
            protoMsg.set_nostreams(cmd_builder.getNoStreams());
            protoMsg.set_timeout(cmd_builder.getTimeout());
            db->updateProtocol(std::vector<events::Message>{protoMsg});

            // Spawn the fts_url_copy
            bool failed = false;
            std::string forkMessage;
            if (-1 == pr.executeProcessShell(forkMessage)) {
                failed = true;
                db->updateTransferStatus(
                    tf.jobId, tf.fileId, 0.0, "FAILED",
                    "Transfer failed to fork, check fts3server.log for more details",
                    (int) pr.getPid(), 0, 0, false
                );
                db->updateJobStatus(tf.jobId, "FAILED", 0);

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
                db->updateTransferStatus(
                    tf.jobId, tf.fileId, 0.0, "ACTIVE", "",
                    (int) pr.getPid(), 0, 0, false
                );
            }

            // Send current state
            SingleTrStateInstance::instance().sendStateMessage(tf.jobId, tf.fileId);
            fts3::events::MessageUpdater msg;
            msg.set_job_id(tf.jobId);
            msg.set_file_id(tf.fileId);
            msg.set_process_id(pr.getPid());
            msg.set_timestamp(milliseconds_since_epoch());

            // Only set watcher when the file has started
            if(!failed) {
                ThreadSafeList::get_instance().push_back(msg);
            }
        }
        else {
            notScheduled.insert(make_pair(source_hostname, destin_hostname));
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
