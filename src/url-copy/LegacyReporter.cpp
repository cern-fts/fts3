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

#include "LegacyReporter.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include "common/Logger.h"
#include "monitoring/msg-ifce.h"

namespace events = fts3::events;
using fts3::common::commit;


static std::string mapErrnoToString(int err)
{
    char buf[256] = {0};
    char const *str = strerror_r(err, buf, sizeof(buf));
    if (str) {
        std::string rep(str);
        std::replace(rep.begin(), rep.end(), ' ', '_');
        return boost::to_upper_copy(rep);
    }
    return "GENERAL ERROR";
}


static std::string replaceMetadataString(std::string text)
{
    text = boost::replace_all_copy(text, "?"," ");
    text = boost::replace_all_copy(text, "\\\"","\"");
    return text;
}


LegacyReporter::LegacyReporter(const UrlCopyOpts &opts): producer(opts.msgDir), opts(opts)
{
}


void LegacyReporter::sendTransferStart(const Transfer &transfer, Gfal2TransferParams&)
{
    // Log file
    events::MessageLog log;

    log.set_timestamp(Transfer::Statistics::timestampMilliseconds());
    log.set_job_id(transfer.jobId);
    log.set_file_id(transfer.fileId);
    log.set_host(fts3::common::getFullHostname());
    log.set_log_path(transfer.logFile);
    log.set_has_debug_file(opts.debugLevel > 1);

    producer.runProducerLog(log);

    // Status
    if (opts.isSessionReuse || opts.isMultihop) {
        events::Message status;

        status.set_timestamp(Transfer::Statistics::timestampMilliseconds());
        status.set_job_id(transfer.jobId);
        status.set_file_id(transfer.fileId);
        status.set_source_se(transfer.source.host);
        status.set_dest_se(transfer.destination.host);
        status.set_process_id(getpid());
        status.set_transfer_status("ACTIVE");

        producer.runProducerStatus(status);
    }

    // Transfer completed
    TransferCompleted completed;

    completed.agent_fqdn = opts.alias;
    completed.transfer_id = transfer.getTransferId();
    completed.endpoint = opts.alias;

    if (transfer.source.protocol == "srm") {
        completed.source_srm_version = "2.2.0";
    }
    if (transfer.destination.protocol == "srm") {
        completed.destination_srm_version = "2.2.0";
    }

    completed.vo = opts.voName;
    completed.source_url = transfer.source.fullUri;
    completed.dest_url = transfer.destination.fullUri;
    completed.source_hostname = transfer.source.getSeName();
    completed.dest_hostname = transfer.destination.getSeName();
    completed.t_channel = transfer.getChannel();
    completed.channel_type = "urlcopy";
    completed.user_dn = replaceMetadataString(opts.userDn);
    completed.file_metadata = replaceMetadataString(transfer.fileMetadata);
    completed.job_metadata = replaceMetadataString(opts.jobMetadata);
    completed.job_m_replica = transfer.isMultipleReplicaJob;
    completed.srm_space_token_source = transfer.sourceTokenDescription;
    completed.srm_space_token_dest = transfer.destTokenDescription;
    completed.tr_timestamp_start = Transfer::Statistics::timestampMilliseconds();

    if (opts.enableMonitoring) {
        std::string msgReturnValue = MsgIfce::getInstance()->SendTransferStartMessage(producer, completed);
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Start message content: " << msgReturnValue << commit;
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
    status.set_timestamp(Transfer::Statistics::timestampMilliseconds());
    status.set_transfer_status("UPDATE");
    status.set_process_id(getpid());

    producer.runProducerStatus(status);
}


void LegacyReporter::sendTransferCompleted(const Transfer &transfer, Gfal2TransferParams &params)
{
    // Log file
    events::MessageLog log;

    log.set_timestamp(Transfer::Statistics::timestampMilliseconds());
    log.set_job_id(transfer.jobId);
    log.set_file_id(transfer.fileId);
    log.set_host(fts3::common::getFullHostname());
    log.set_log_path(transfer.logFile);
    log.set_has_debug_file(opts.debugLevel > 1);

    producer.runProducerLog(log);

    // Status
    events::Message status;

    status.set_timestamp(Transfer::Statistics::timestampMilliseconds());
    status.set_job_id(transfer.jobId);
    status.set_file_id(transfer.fileId);
    status.set_source_se(transfer.source.host);
    status.set_dest_se(transfer.destination.host);
    status.set_process_id(getpid());

    if (transfer.error) {
        std::stringstream fullErrMsg;
        fullErrMsg << transfer.error->scope() << " " << transfer.error->what();
        status.set_transfer_status("FAILED");
        status.set_transfer_message(fullErrMsg.str());
        status.set_retry(transfer.error->isRecoverable());
    }
    else {
        status.set_transfer_status("FINISHED");
    }

    producer.runProducerStatus(status);

    // Transfer completed
    TransferCompleted completed;

    completed.agent_fqdn = opts.alias;
    completed.transfer_id = transfer.getTransferId();
    completed.endpoint = opts.alias;

    if (transfer.source.protocol == "srm") {
        completed.source_srm_version = "2.2.0";
    }
    if (transfer.destination.protocol == "srm") {
        completed.destination_srm_version = "2.2.0";
    }

    completed.vo = opts.voName;
    completed.source_url = transfer.source.fullUri;
    completed.dest_url = transfer.destination.fullUri;
    completed.source_hostname = transfer.source.getSeName();
    completed.dest_hostname = transfer.destination.getSeName();
    completed.t_channel = transfer.getChannel();
    completed.channel_type = "urlcopy";
    completed.user_dn = replaceMetadataString(opts.userDn);
    completed.file_metadata = replaceMetadataString(transfer.fileMetadata);
    completed.job_metadata = replaceMetadataString(opts.jobMetadata);
    completed.job_m_replica = transfer.isMultipleReplicaJob;
    completed.srm_space_token_source = transfer.sourceTokenDescription;
    completed.srm_space_token_dest = transfer.destTokenDescription;

    completed.job_m_replica = opts.isMultipleReplicaJob;
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
            }
            else {
                completed.job_state = "ACTIVE";
            }
        }
        else {
            completed.job_state = "UNKNOWN";
        }
    }
    else {
        completed.final_transfer_state = "Ok";
        if(transfer.isMultipleReplicaJob) {
            completed.job_state = "FINISHED";
        }
        else {
            completed.job_state = "UNKNOWN";
        }
    }

    completed.total_bytes_transferred = transfer.transferredBytes;
    completed.number_of_streams = params.getNumberOfStreams();
    completed.tcp_buffer_size = params.getTcpBuffersize();
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

    if (opts.enableMonitoring) {
        std::string msgReturnValue = MsgIfce::getInstance()->SendTransferFinishMessage(producer, completed);
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Start message content: " << msgReturnValue << commit;
    }
}


void LegacyReporter::sendPing(const Transfer &transfer)
{
    events::MessageUpdater ping;
    ping.set_timestamp(Transfer::Statistics::timestampMilliseconds());
    ping.set_job_id(transfer.jobId);
    ping.set_file_id(transfer.fileId);
    ping.set_transfer_status("ACTIVE");
    ping.set_source_surl(transfer.source.fullUri);
    ping.set_dest_surl(transfer.destination.fullUri);
    ping.set_process_id(getpid());
    ping.set_throughput(transfer.throughput);
    ping.set_transferred(transfer.transferredBytes);
    ping.set_source_turl("gsiftp:://fake");
    ping.set_dest_turl("gsiftp:://fake");

    producer.runProducerStall(ping);
}
