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

#include "Reporter.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace events = fts3::events;


static uint64_t timestampMilliseconds()
{
    return time(NULL) * 1000;
}


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


Reporter::~Reporter()
{
}


DefaultReporter::DefaultReporter(const UrlCopyOpts &opts): producer(opts.msgDir), opts(opts)
{
}


void DefaultReporter::sendTransferStart(const Transfer &transfer, Gfal2TransferParams &params)
{
    events::TransferStart start;

    start.set_timestamp(timestampMilliseconds());
    start.set_agent(fts3::common::getFullHostname());
    start.set_job_id(transfer.jobId);
    start.set_file_id(transfer.fileId);
    start.set_transfer_id(transfer.getTransferId());
    start.set_vo_name(opts.voName);
    start.set_user_dn(opts.userDn);

    start.mutable_source()->set_hostname(transfer.source.host);
    start.mutable_source()->set_surl(transfer.source.fullUri);
    start.mutable_source()->set_srm_space_token(transfer.sourceTokenDescription);

    start.mutable_dest()->set_hostname(transfer.destination.host);
    start.mutable_dest()->set_surl(transfer.destination.fullUri);
    start.mutable_dest()->set_srm_space_token(transfer.destTokenDescription);


    if (transfer.source.protocol == "srm") {
        start.mutable_source()->set_srm_version("2.2.0");
    }
    if (transfer.destination.protocol == "srm") {
        start.mutable_dest()->set_srm_version("2.2.0");
    }

    start.set_channel(transfer.getChannel());
    start.set_process_id(getpid());
    start.set_filesize(transfer.fileSize);

    start.set_file_metadata(transfer.fileMetadata);
    start.set_job_metadata(opts.jobMetadata);

    start.set_log_file(transfer.logFile);

    //producer.runProducerStart(start);
}


void DefaultReporter::sendProtocol(const Transfer &transfer, Gfal2TransferParams &params)
{
    // TODO
}


void DefaultReporter::sendTransferCompleted(const Transfer &transfer, Gfal2TransferParams &params)
{
    events::TransferCompleted completed;

    completed.set_timestamp(timestampMilliseconds());
    completed.set_agent(fts3::common::getFullHostname());
    completed.set_transfer_id(transfer.getTransferId());
    completed.set_vo_name(opts.voName);
    completed.set_user_dn(opts.userDn);

    completed.mutable_source()->set_hostname(transfer.source.host);
    completed.mutable_source()->set_surl(transfer.source.fullUri);
    completed.mutable_source()->set_srm_space_token(transfer.sourceTokenDescription);

    completed.mutable_dest()->set_hostname(transfer.destination.host);
    completed.mutable_dest()->set_surl(transfer.destination.fullUri);
    completed.mutable_dest()->set_srm_space_token(transfer.destTokenDescription);

    if (transfer.source.protocol == "srm") {
        completed.mutable_source()->set_srm_version("2.2.0");
    }
    if (transfer.destination.protocol == "srm") {
        completed.mutable_dest()->set_srm_version("2.2.0");
    }

    completed.mutable_transfer();
    completed.mutable_source_checksum();
    completed.mutable_dest_checksum();
    completed.mutable_srm_preparation();
    completed.mutable_srm_finalization();
    completed.mutable_total_process();

    completed.set_transfer_timeout(params.getTimeout());
    completed.set_checksum_timeout(0);

    if (transfer.isMultipleReplicaJob) {
        if (transfer.error && transfer.isLastReplica) {
            completed.set_job_state("FAILED");
        }
        else if (transfer.error && !transfer.isLastReplica) {
            completed.set_job_state("ACTIVE");
        }
        else {
            completed.set_job_state("FINISHED");
        }
    }
    else {
        completed.set_job_state("UNKNOWN");
    }

    if (transfer.error) {
        completed.set_final_transfer_state("Error");
        completed.set_transfer_error_code(transfer.error->code());
        completed.set_transfer_error_scope(transfer.error->scope());
        completed.set_transfer_error_message(transfer.error->what());
        completed.set_failure_phase(transfer.error->scope());
        completed.set_transfer_error_category(mapErrnoToString(transfer.error->code()));
        completed.set_is_recoverable(transfer.error->isRecoverable());
    }


    completed.set_total_bytes_transfered(transfer.transferredBytes);
    completed.set_number_of_streams(params.getNumberOfStreams());
    completed.set_tcp_buffer_size(params.getTcpBuffersize());
    completed.set_block_size(0);
    completed.set_file_size(transfer.fileSize);

    completed.set_file_metadata(transfer.fileMetadata);
    completed.set_job_metadata(opts.jobMetadata);

    completed.set_is_job_multiple_replica(transfer.isMultipleReplicaJob);
    completed.set_log_file(transfer.logFile);

    //producer.runProducerCompleted(start);
}


void DefaultReporter::sendPing(const Transfer &transfer)
{
    events::MessageUpdater ping;
    ping.set_timestamp(timestampMilliseconds());
    ping.set_job_id(transfer.jobId);
    ping.set_file_id(transfer.fileId);
    ping.set_transfer_status("UPDATE");
    ping.set_source_surl(transfer.source.fullUri);
    ping.set_dest_surl(transfer.destination.fullUri);
    ping.set_process_id(getpid());
    ping.set_throughput(transfer.throughput);
    ping.set_transferred(transfer.transferredBytes);

    producer.runProducerStall(ping);
}
