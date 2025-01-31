/*
 * Copyright (c) CERN 2016
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

#include <boost/thread.hpp>

#include "LegacyReporter.h"
#include "common/Logger.h"
#include "common/TimeUtils.h"
#include "monitoring/msg-ifce.h"
#include "heuristics.h"

namespace events = fts3::events;
using fts3::common::commit;


LegacyReporter::LegacyReporter(const UrlCopyOpts &opts): producer(opts.msgDir), opts(opts),
    zmqContextPing(1), zmqContextToken(1),
    zmqPingSocket(zmqContextPing, zmq::socket_type::pub),
    zmqTokenSocket(zmqContextToken, zmq::socket_type::req)
{
    auto pingAddress = "ipc://" + opts.msgDir + "/url_copy-ping.ipc";
    auto tokenAddress = "ipc://" + opts.msgDir + "/url_copy-token-refresh.ipc";
    zmqPingSocket.connect(pingAddress.c_str());
    // Wait at most 1 minute for token ZMQ communication
    int tokenSocketTimeout_ms = 60 * 1000;
    zmqTokenSocket.set(zmq::sockopt::rcvtimeo, tokenSocketTimeout_ms);
    zmqTokenSocket.set(zmq::sockopt::sndtimeo, tokenSocketTimeout_ms);
    zmqTokenSocket.set(zmq::sockopt::req_relaxed, 1);
    zmqTokenSocket.connect(tokenAddress.c_str());
}


void LegacyReporter::sendTransferStart(const Transfer &transfer, Gfal2TransferParams&)
{
    // Log file
    events::MessageLog log;

    log.set_timestamp(millisecondsSinceEpoch());
    log.set_job_id(transfer.jobId);
    log.set_file_id(transfer.fileId);
    log.set_host(fts3::common::getFullHostname());
    log.set_log_path(transfer.logFile);
    log.set_has_debug_file(opts.debugLevel > 1);

    producer.runProducerLog(log);

    // Status
    events::Message status;

    status.set_timestamp(millisecondsSinceEpoch());
    status.set_job_id(transfer.jobId);
    status.set_file_id(transfer.fileId);
    status.set_source_se(transfer.source.host);
    status.set_dest_se(transfer.destination.host);
    status.set_process_id(getpid());
    status.set_transfer_status("ACTIVE");

    producer.runProducerStatus(status);

    // Fill transfer started
    TransferCompleted started;

    started.transfer_id = transfer.getTransferId();
    started.job_id = transfer.jobId;
    started.file_id = transfer.fileId;
    started.endpoint = opts.alias;

    if (transfer.source.protocol == "srm") {
        started.source_srm_version = "2.2.0";
    }
    if (transfer.destination.protocol == "srm") {
        started.destination_srm_version = "2.2.0";
    }

    started.vo = opts.voName;
    started.source_url = transfer.source.fullUri;
    started.dest_url = transfer.destination.fullUri;
    started.source_hostname = transfer.source.host;
    started.dest_hostname = transfer.destination.host;
    started.t_channel = transfer.getChannel();
    started.channel_type = "urlcopy";
    started.user_dn = replaceMetadataString(opts.userDn);
    started.file_metadata = replaceMetadataString(transfer.fileMetadata);
    started.job_metadata = replaceMetadataString(opts.jobMetadata);
    started.srm_space_token_source = transfer.sourceTokenDescription;
    started.srm_space_token_dest = transfer.destTokenDescription;
    started.tr_timestamp_start = millisecondsSinceEpoch();

    if (opts.enableMonitoring) {
        std::string msgReturnValue = MsgIfce::getInstance()->SendTransferStartMessage(producer, started);
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Transfer start message content: " << msgReturnValue << commit;
    }
}


void LegacyReporter::sendProtocol(const Transfer &transfer, Gfal2TransferParams &params)
{
    events::Message status;

    status.set_job_id(transfer.jobId);
    status.set_file_id(transfer.fileId);
    status.set_source_se(transfer.source.host);
    status.set_dest_se(transfer.destination.host);
    status.set_filesize(transfer.fileSize);
    status.set_nostreams(params.getNumberOfStreams());
    status.set_timeout(params.getTimeout());
    status.set_buffersize(params.getTcpBuffersize());
    status.set_timestamp(millisecondsSinceEpoch());
    status.set_transfer_status("UPDATE");
    status.set_process_id(getpid());

    producer.runProducerStatus(status);
}


void LegacyReporter::sendTransferCompleted(const Transfer &transfer, Gfal2TransferParams &params)
{
    // Log file
    events::MessageLog log;

    log.set_timestamp(millisecondsSinceEpoch());
    log.set_job_id(transfer.jobId);
    log.set_file_id(transfer.fileId);
    log.set_host(fts3::common::getFullHostname());
    log.set_log_path(transfer.logFile);
    log.set_has_debug_file(opts.debugLevel > 1);

    producer.runProducerLog(log);

    // Status
    events::Message status;

    status.set_timestamp(millisecondsSinceEpoch());
    status.set_job_id(transfer.jobId);
    status.set_file_id(transfer.fileId);
    status.set_source_se(Uri::parse(transfer.source.fullUri).getSeName());
    status.set_dest_se(Uri::parse(transfer.destination.fullUri).getSeName());
    status.set_process_id(getpid());
    status.set_filesize(transfer.fileSize);
    status.set_time_in_secs(transfer.getTransferDurationInSeconds());

    if (transfer.error) {
        std::stringstream fullErrMsg;
        fullErrMsg << transfer.error->scope() << " [" << transfer.error->code() << "] " << transfer.error->what();

        if (transfer.error->code() == ECANCELED) {
            status.set_transfer_status("CANCELED");
        } else {
            status.set_transfer_status("FAILED");

            if ((transfer.error->code() == EEXIST) && (opts.dstFileReport) && (!opts.overwrite)) {
                status.set_file_metadata(replaceMetadataString(transfer.fileMetadata));
            }

            status.set_log_path(transfer.logFile);
        }
        status.set_transfer_message(fullErrMsg.str());
        status.set_retry(transfer.error->isRecoverable());
        status.set_errcode(transfer.error->code());
    } else {
        status.set_errcode(0);
        status.set_transfer_status("FINISHED");

        status.set_gfal_perf_timestamp(transfer.stats.transfer.start + transfer.stats.elapsedAtPerf);

        if (transfer.transferredBytes < transfer.previousPingTransferredBytes) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Transferred bytes decreased:"
                                               << " transferred=" << transfer.transferredBytes
                                               << " previous_transferred=" << transfer.previousPingTransferredBytes
                                               << commit;
            status.set_transferred_since_last_ping(0);
        } else if (transfer.fileSize < transfer.previousPingTransferredBytes) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Transferred more bytes than file size:"
                                               << " file_size=" << transfer.fileSize
                                               << " previous_transferred=" << transfer.previousPingTransferredBytes
                                               << commit;
            status.set_transferred_since_last_ping(0);
        }
        else {
            status.set_transferred_since_last_ping(transfer.fileSize - transfer.previousPingTransferredBytes);
        }

        // Message Throughput in MiB/sec
        if (transfer.averageThroughput > 0) { // Throughput from Gfal PerformanceCallback
            if (transfer.instantaneousThroughput > 10000000000 || transfer.averageThroughput > 10000000000) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Throughput nonsensical, setting throughput and instantaneous throughput to zero:"
                                                   << " average=" << transfer.averageThroughput
                                                   << " instantaneous=" << transfer.instantaneousThroughput
                                                   << commit;
                status.set_throughput(0.0);
                status.set_instantaneous_throughput(0.0);
            } else {
                status.set_throughput(transfer.averageThroughput / 1024.0);
                status.set_instantaneous_throughput(transfer.instantaneousThroughput / 1024.0);
            }
        } else { // Throughput must be computed manually (short transfers)
            double transferDuration = transfer.getTransferDurationInSeconds();

            if (transferDuration > 0) {
                double transferred = static_cast<double>(transfer.fileSize) / 1048576.0; // in MiB
                double throughput = transferred / transferDuration; // in MiB/s
                status.set_throughput(throughput);
                status.set_instantaneous_throughput(throughput);
            } else {
                status.set_throughput(0.0);
                status.set_instantaneous_throughput(0.0);
            }
        }
    }

    producer.runProducerStatus(status);

    // Fill transfer completed
    TransferCompleted completed;

    completed.transfer_id = transfer.getTransferId();
    completed.job_id = transfer.jobId;
    completed.file_id = transfer.fileId;
    completed.endpoint = opts.alias;

    if (transfer.source.protocol == "srm") {
        completed.source_srm_version = "2.2.0";
    }
    if (transfer.destination.protocol == "srm") {
        completed.destination_srm_version = "2.2.0";
    }

    std::string protocol = transfer.destination.protocol;
    if (protocol.empty()) {
        protocol = transfer.source.protocol;
    }

    completed.vo = opts.voName;
    completed.source_url = transfer.source.fullUri;
    completed.dest_url = transfer.destination.fullUri;
    completed.source_hostname = transfer.source.host;
    completed.dest_hostname = transfer.destination.host;
    completed.source_se = transfer.source.getSeName();
    completed.dest_se = transfer.destination.getSeName();
    completed.protocol = protocol;
    completed.t_channel = transfer.getChannel();
    completed.channel_type = "urlcopy";
    completed.user_dn = replaceMetadataString(opts.userDn);
    completed.file_metadata = replaceMetadataString(transfer.fileMetadata);
    completed.job_metadata = replaceMetadataString(opts.jobMetadata);
    completed.activity = transfer.activity;
    completed.job_m_replica = transfer.isMultipleReplicaJob;
    completed.job_multihop = transfer.isMultihopJob;
    completed.is_lasthop = transfer.isLastHop;
    completed.is_archiving = transfer.isArchiving;
    completed.srm_space_token_source = transfer.sourceTokenDescription;
    completed.srm_space_token_dest = transfer.destTokenDescription;
    completed.transfer_timeout = params.getTimeout();

    if (transfer.error) {
        completed.transfer_error_code = transfer.error->code();
        completed.transfer_error_scope = transfer.error->scope();
        completed.transfer_error_message = transfer.error->what();
        completed.failure_phase = transfer.error->phase();
        completed.transfer_error_category = mapErrnoToString(transfer.error->code());
        if (transfer.error->code() == ECANCELED) {
            completed.final_transfer_state = "Abort";
        }
        else {
            completed.final_transfer_state = "Error";
        }
        completed.retry = opts.retry;
        completed.retry_max = opts.retryMax;
        completed.is_recoverable = transfer.error->isRecoverable();

        if (transfer.isMultipleReplicaJob) {
            if (transfer.isLastReplica) {
                completed.job_state = "FAILED";
            } else {
                completed.job_state = "ACTIVE";
            }
        } else if (transfer.isMultihopJob) {
            completed.job_state = "FAILED";
        } else {
            completed.job_state = "UNKNOWN";
        }
    }
    else {
        completed.final_transfer_state = "Ok";

        if (transfer.isMultipleReplicaJob) {
            completed.job_state = "FINISHED";
        } else if (transfer.isMultihopJob) {
            if (transfer.isLastHop) {
                completed.job_state = "FINISHED";
            } else {
                completed.job_state = "ACTIVE";
            }
        } else {
            completed.job_state = "UNKNOWN";
        }
    }

    if (completed.final_transfer_state == "Ok") {
        completed.final_transfer_state_flag = 1;
    } else if (completed.final_transfer_state == "Error") {
        completed.final_transfer_state_flag = 0;
    }

    completed.total_bytes_transferred = transfer.transferredBytes;
    completed.number_of_streams = params.getNumberOfStreams();
    completed.tcp_buffer_size = params.getTcpBuffersize();
    completed.scitag = transfer.scitag;
    completed.file_size = transfer.fileSize;

    completed.timestamp_transfer_started = transfer.stats.transfer.start;
    completed.timestamp_transfer_completed = transfer.stats.transfer.end;
    completed.timestamp_checksum_source_started = transfer.stats.sourceChecksum.start;
    completed.timestamp_checksum_source_ended = transfer.stats.sourceChecksum.end;
    completed.timestamp_checksum_dest_started = transfer.stats.destChecksum.start;
    completed.timestamp_checksum_dest_ended = transfer.stats.destChecksum.end;

    completed.time_spent_in_srm_preparation_start = transfer.stats.srmPreparation.start;
    completed.time_spent_in_srm_preparation_end = transfer.stats.srmPreparation.end;
    completed.time_spent_in_srm_finalization_start = transfer.stats.srmFinalization.start;
    completed.time_spent_in_srm_finalization_end = transfer.stats.srmFinalization.end;
    completed.tr_timestamp_start = transfer.stats.process.start;
    completed.tr_timestamp_complete = transfer.stats.process.end;

    completed.transfer_time_ms = transfer.stats.transfer.end - transfer.stats.transfer.start;
    completed.operation_time_ms = transfer.stats.process.end - transfer.stats.process.start;
    completed.throughput_bps = (completed.transfer_time_ms > 0) ? ((double) completed.file_size / ((double) completed.transfer_time_ms / 1000.0)) : -1;

    completed.srm_preparation_time_ms = transfer.stats.srmPreparation.end - transfer.stats.srmPreparation.start;
    completed.srm_finalization_time_ms = transfer.stats.srmFinalization.end - transfer.stats.srmFinalization.start;
    completed.srm_overhead_time_ms = completed.srm_preparation_time_ms + completed.srm_finalization_time_ms;
    completed.srm_overhead_percentage = (completed.operation_time_ms > 0) ? ((double) completed.srm_overhead_time_ms * 100 / (double) completed.operation_time_ms) : -1;

    completed.checksum_source_time_ms = transfer.stats.sourceChecksum.end - transfer.stats.sourceChecksum.start;
    completed.checksum_dest_time_ms = transfer.stats.destChecksum.end - transfer.stats.destChecksum.start;

    // 'ipver' field keyword ("ipv6" | "ipv4" | "unknown")
    completed.ipver = Transfer::IPverToIPv6String(transfer.stats.ipver);
    completed.eviction_code = transfer.stats.evictionRetc;
    completed.cleanup_code = transfer.stats.cleanupRetc;
    completed.final_destination = transfer.stats.finalDestination;
    completed.transfer_type = transfer.stats.transferType;
    completed.auth_method = opts.authMethod;
    completed.overwrite_on_disk_flag = opts.overwriteOnDisk;
    completed.overwrite_on_disk_deletion_code = transfer.stats.overwriteOnDiskRetc;

    if (opts.enableMonitoring) {
        auto msgReturnValue = MsgIfce::getInstance()->SendTransferFinishMessage(producer, completed);
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Transfer complete message content: " << msgReturnValue << commit;
    }
}


void LegacyReporter::sendPing(Transfer &transfer)
{
    if (transfer.transferredBytes < transfer.previousPingTransferredBytes) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Transferred bytes decreased, not sending perf to server:"
                                           << " transferred=" << transfer.transferredBytes
                                           << " previous_transferred=" << transfer.previousPingTransferredBytes
                                           << commit;
        return;
    }

    if (transfer.instantaneousThroughput > 10000000000 || transfer.averageThroughput > 10000000000) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Throughput nonsensical, not sending perf to server:"
                                           << " average=" << transfer.averageThroughput
                                           << " instantaneous=" << transfer.instantaneousThroughput
                                           << commit;
        return;
    }

    events::MessageUpdater ping;
    ping.set_timestamp(millisecondsSinceEpoch());
    // Note: elapsedAtPerf does not start when transfer.start is set. (gfal bug)
    ping.set_gfal_perf_timestamp(transfer.stats.transfer.start + transfer.stats.elapsedAtPerf);
    ping.set_job_id(transfer.jobId);
    ping.set_file_id(transfer.fileId);
    ping.set_transfer_status("ACTIVE");
    ping.set_source_surl(transfer.source.fullUri);
    ping.set_dest_surl(transfer.destination.fullUri);
    ping.set_process_id(getpid());
    ping.set_throughput(transfer.averageThroughput / 1024.0);
    ping.set_instantaneous_throughput(transfer.instantaneousThroughput / 1024.0);
    ping.set_transferred(transfer.transferredBytes);
    ping.set_transferred_since_last_ping(transfer.transferredBytes - transfer.previousPingTransferredBytes);
    ping.set_source_turl("gsiftp:://fake");
    ping.set_dest_turl("gsiftp:://fake");

    try {
        std::string serialized = ping.SerializeAsString();
        zmq::message_t message(serialized.size());
        memcpy(message.data(), serialized.c_str(), serialized.size());
        zmqPingSocket.send(message, zmq::send_flags::none);
    }
    catch (const std::exception &error) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to send heartbeat: " << error.what() << commit;
    }
    transfer.previousPingTransferredBytes = transfer.transferredBytes;
}

std::pair<std::string, int64_t> LegacyReporter::requestTokenRefresh(const std::string& token_id, const Transfer& transfer)
{
    events::TokenRefreshRequest request;
    events::TokenRefreshResponse response;

    request.set_token_id(token_id);
    request.set_job_id(transfer.jobId);
    request.set_file_id(transfer.fileId);
    request.set_hostname(fts3::common::getFullHostname());
    request.set_process_id(getpid());

    int attempts{0};
    auto end = getTimestampSeconds(300);
    auto nextAttempt = getTimestampSeconds(60);

    while (true) {
        auto now = getTimestampSeconds();

        if (now >= end) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Aborting token-refresh request. "
                                               << "Total operation time exceeded 300s! (attempts=" << attempts << ")"
                                               << commit;
            break;
        }

        if ((attempts > 0) && (now < nextAttempt)) {
            auto interval = std::min(nextAttempt, end) - now;
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Token-refresh request: Will sleep for " << interval << " seconds!" << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(interval));
            now = getTimestampSeconds();
        }

        if (now >= nextAttempt) {
            nextAttempt = getTimestampSeconds(60);
        }

        try {
            std::ostringstream msg;
            msg << "Sending token-refresh request: token_id=" << token_id;
            if (++attempts > 1) { msg << " (attempt=" << attempts << ")"; }
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << msg.str() << commit;

            auto serialized = request.SerializeAsString();
            zmqTokenSocket.send(zmq::message_t{serialized}, zmq::send_flags::none);

            zmq::message_t reply;
            auto recv_result = zmqTokenSocket.recv(reply, zmq::recv_flags::none);

            if (!recv_result) {
                if (zmq_errno() == EAGAIN) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Token-refresh request: waiting for response timed out!" << commit;
                } else {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Token-refresh request: failure receiving response ("
                                                       << zmq_strerror(zmq_errno()) << ")" << commit;
                }

                continue;
            }

            if (!response.ParseFromArray(reply.data(), static_cast<int>(reply.size()))) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Failed to parse token-refresh response!" << commit;
                continue;
            }

            if (response.response_type() == events::TokenRefreshResponse::TYPE_REFRESH_FAILURE) {
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Received failed-refresh response: \"" << response.refresh_message() << "\""
                                                   << " (refresh_timestamp=" << response.refresh_timestamp() << ")"
                                                   << commit;
                continue;
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Received token-refresh response: "
                                            << accessTokenPayload(response.access_token())
                                            << commit;

            return {response.access_token(), response.expiry_timestamp()};
        } catch (const std::exception& error) {
            FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Failed to send/receive token-refresh request: " << error.what() << commit;
        }
    }

    return {"", 0};
}
