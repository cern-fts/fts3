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

#include <chrono>
#include <iomanip>
#include <fstream>
#include <cajun/json/elements.h>
#include <cajun/json/writer.h>
#include "msg-ifce.h"
#include "common/Logger.h"

bool MsgIfce::instanceFlag = false;
MsgIfce *MsgIfce::single = NULL;


static uint64_t getTimestampMillisecs()
{
    std::chrono::milliseconds timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()
        );

    return timestamp.count();
}


static std::string ReplaceNonPrintableCharacters(const std::string &s)
{
    std::string result;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        int AsciiValue = static_cast<int> (c);
        if (AsciiValue < 32 || AsciiValue > 127) {
            result.append(" ");
        }
        else {
            result += s.at(i);
        }
    }
    return result;
}


MsgIfce *MsgIfce::getInstance()
{
    if (!instanceFlag) {
        single = new MsgIfce();
        instanceFlag = true;
        return single;
    }
    else {
        return single;
    }
}


MsgIfce::MsgIfce()
{
}


MsgIfce::~MsgIfce()
{
    instanceFlag = false;
}


static void set_metadata(json::Object &json, const std::string &key, const std::string &value)
{
    if (!value.empty()) {
        try {
            std::istringstream valueStream(value);
            json::UnknownElement metadata;
            json::Reader::Read(metadata, valueStream);
            json[key] = metadata;
            return;
        }
        catch (...) {
            // continue
        }
    }

    json[key] = json::String(value);
}


std::string MsgIfce::SendTransferStartMessage(Producer &producer, const TransferCompleted &tr_started)
{
    json::Object message;

    message["transfer_id"] = json::String(tr_started.transfer_id);
    message["job_id"] = json::String(tr_started.job_id);
    message["file_id"] = json::Number(tr_started.file_id);
    message["endpnt"] = json::String(tr_started.endpoint);
    message["timestamp"] = json::Number(getTimestampMillisecs());
    message["src_srm_v"] = json::String(tr_started.source_srm_version);
    message["dest_srm_v"] = json::String(tr_started.destination_srm_version);
    message["vo"] = json::String(tr_started.vo);
    message["src_url"] = json::String(tr_started.source_url);
    message["dst_url"] = json::String(tr_started.dest_url);
    message["src_hostname"] = json::String(tr_started.source_hostname);
    message["dst_hostname"] = json::String(tr_started.dest_hostname);
    message["src_site_name"] = json::String(tr_started.source_site_name);
    message["dst_site_name"] = json::String(tr_started.dest_site_name);
    message["t_channel"] = json::String(tr_started.t_channel);
    message["srm_space_token_src"] = json::String(tr_started.srm_space_token_source);
    message["srm_space_token_dst"] = json::String(tr_started.srm_space_token_dest);
    message["user_dn"] = json::String(tr_started.user_dn);

    if (tr_started.file_metadata != "x") {
        set_metadata(message, "file_metadata", tr_started.file_metadata);
    }
    else {
        message["file_metadata"] = json::String();
    }

    set_metadata(message, "job_metadata", tr_started.job_metadata);

    std::ostringstream stream;

    stream << "ST ";
    json::Writer::Write(message, stream);

    std::string msgStr = stream.str();
    int errCode = producer.runProducerMonitoring(msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendTransferFinishMessage(Producer &producer, const TransferCompleted &tr_completed)
{
    json::Object message;

    message["tr_id"] = json::String(tr_completed.transfer_id);
    message["job_id"] = json::String(tr_completed.job_id);
    message["file_id"] = json::Number(tr_completed.file_id);
    message["endpnt"] = json::String(tr_completed.endpoint);
    message["src_srm_v"] = json::String(tr_completed.source_srm_version);
    message["dest_srm_v"] = json::String(tr_completed.destination_srm_version);
    message["vo"] = json::String(tr_completed.vo);
    message["src_url"] = json::String(tr_completed.source_url);
    message["dst_url"] = json::String(tr_completed.dest_url);
    message["src_hostname"] = json::String(tr_completed.source_hostname);
    message["dst_hostname"] = json::String(tr_completed.dest_hostname);
    message["src_se"] = json::String(tr_completed.source_se);
    message["dst_se"] = json::String(tr_completed.dest_se);
    message["protocol"] = json::String(tr_completed.protocol);
    message["src_site_name"] = json::String(tr_completed.source_site_name);
    message["dst_site_name"] = json::String(tr_completed.dest_site_name);
    message["t_channel"] = json::String(tr_completed.t_channel);
    message["timestamp_tr_st"] = json::Number(tr_completed.timestamp_transfer_started);
    message["timestamp_tr_comp"] = json::Number(tr_completed.timestamp_transfer_completed);
    message["timestamp_chk_src_st"] = json::Number(tr_completed.timestamp_checksum_source_started);
    message["timestamp_chk_src_ended"] = json::Number(tr_completed.timestamp_checksum_source_ended);
    message["timestamp_checksum_dest_st"] = json::Number(tr_completed.timestamp_checksum_dest_started);
    message["timestamp_checksum_dest_ended"] = json::Number(tr_completed.timestamp_checksum_dest_ended);
    message["t_timeout"] = json::Number(tr_completed.transfer_timeout);
    message["chk_timeout"] = json::Number(tr_completed.checksum_timeout);
    message["t_error_code"] = json::Number(tr_completed.transfer_error_code);
    message["tr_error_scope"] = json::String(tr_completed.transfer_error_scope);
    message["t_failure_phase"] = json::String(tr_completed.failure_phase);
    message["tr_error_category"] = json::String(tr_completed.transfer_error_category);
    message["t_final_transfer_state"] = json::String(tr_completed.final_transfer_state);
    message["t_final_transfer_state_flag"] = json::Number(tr_completed.final_transfer_state_flag);
    message["tr_bt_transfered"] = json::Number(tr_completed.total_bytes_transferred);
    message["nstreams"] = json::Number(tr_completed.number_of_streams);
    message["buf_size"] = json::Number(tr_completed.tcp_buffer_size);
    message["tcp_buf_size"] = json::Number(tr_completed.tcp_buffer_size);
    message["block_size"] = json::Number(tr_completed.block_size);
    // Prepare to drop "f_size" field in the future
    message["f_size"] = json::Number(tr_completed.file_size);
    message["file_size"] = json::Number(tr_completed.file_size);
    message["time_srm_prep_st"] = json::Number(tr_completed.time_spent_in_srm_preparation_start);
    message["time_srm_prep_end"] = json::Number(tr_completed.time_spent_in_srm_preparation_end);
    message["time_srm_fin_st"] = json::Number(tr_completed.time_spent_in_srm_finalization_start);
    message["time_srm_fin_end"] = json::Number(tr_completed.time_spent_in_srm_finalization_end);
    message["srm_space_token_src"] = json::String(tr_completed.srm_space_token_source);
    message["srm_space_token_dst"] = json::String(tr_completed.srm_space_token_dest);

    std::string temp = ReplaceNonPrintableCharacters(tr_completed.transfer_error_message);
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\''), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\"'), temp.end());
    if (temp.size() > 1024) {
        temp.erase(1024);
    }

    message["t__error_message"] = json::String(temp);
    message["tr_timestamp_start"] = json::Number(tr_completed.tr_timestamp_start);

    if (tr_completed.tr_timestamp_complete) {
        message["tr_timestamp_complete"] = json::Number(tr_completed.tr_timestamp_complete);
    } else {
        message["tr_timestamp_complete"] = json::Number(getTimestampMillisecs());
    }

    message["transfer_time"] = json::Number(tr_completed.transfer_time_ms);
    message["operation_time"] = json::Number(tr_completed.operation_time_ms);
    message["throughput"] = json::Number(tr_completed.throughput_bps);

    message["srm_preparation_time"] = json::Number(tr_completed.srm_preparation_time_ms);
    message["srm_finalization_time"] = json::Number(tr_completed.srm_finalization_time_ms);
    message["srm_overhead_time"] = json::Number(tr_completed.srm_overhead_time_ms);
    message["srm_overhead_percentage"] = json::Number(tr_completed.srm_overhead_percentage);

    message["timestamp_checksum_src_diff"] = json::Number(tr_completed.checksum_source_time_ms);
    message["timestamp_checksum_dst_diff"] = json::Number(tr_completed.checksum_dest_time_ms);

    message["channel_type"] = json::String(tr_completed.channel_type);
    message["user_dn"] = json::String(tr_completed.user_dn);

    if (tr_completed.file_metadata != "x") {
        set_metadata(message, "file_metadata", tr_completed.file_metadata);
    }
    else {
        message["file_metadata"] = json::String("");
    }

    set_metadata(message, "job_metadata", tr_completed.job_metadata);
    message["retry"] = json::Number(tr_completed.retry);
    message["retry_max"] = json::Number(tr_completed.retry_max);
    message["job_m_replica"] = json::Boolean(tr_completed.job_m_replica);
    message["job_multihop"] = json::Boolean(tr_completed.job_multihop);
    message["transfer_lasthop"] = json::Boolean(tr_completed.is_lasthop);
    message["is_archiving"] = json::Boolean(tr_completed.is_archiving);
    message["job_state"] = json::String(tr_completed.job_state);
    message["is_recoverable"] = json::Boolean(tr_completed.is_recoverable);
    message["ipv6"] = json::Boolean(tr_completed.ipv6);
    message["ipver"] = json::String(tr_completed.ipver);
    message["eviction_code"] = json::Number(tr_completed.eviction_code);
    message["cleanup_code"] = json::Number(tr_completed.cleanup_code);
    message["final_destination"] = json::String(tr_completed.final_destination);
    message["transfer_type"] = json::String(tr_completed.transfer_type);

    std::ostringstream stream;

    stream << "CO ";
    json::Writer::Write(message, stream);

    std::string msgStr = stream.str();
    int errCode = producer.runProducerMonitoring(msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendTransferStatusChange(Producer &producer, const TransferState &tr_state)
{
    json::Object message;

    message["user_dn"] = json::String(tr_state.user_dn);
    message["src_url"] = json::String(tr_state.source_url);
    message["dst_url"] = json::String(tr_state.dest_url);
    message["vo_name"] = json::String(tr_state.vo_name);
    message["source_se"] = json::String(tr_state.source_se);
    message["dest_se"] = json::String(tr_state.dest_se);
    message["job_id"] = json::String(tr_state.job_id);
    message["file_id"] = json::Number(tr_state.file_id);
    message["job_state"] = json::String(tr_state.job_state);
    message["file_state"] = json::String(tr_state.file_state);
    message["retry_counter"] = json::Number(tr_state.retry_counter);
    message["retry_max"] = json::Number(tr_state.retry_max);
    message["user_filesize"] = json::Number(tr_state.user_filesize);
    message["timestamp"] = json::Number(tr_state.timestamp);
    message["submit_time"] = json::Number(tr_state.submit_time);
    message["staging_start"] = json::Number(tr_state.staging_start);
    message["staging_finished"] = json::Number(tr_state.staging_finished);
    message["staging"] = json::Boolean(tr_state.staging);
    message["archiving_start"] = json::Number(tr_state.archiving_start);
    message["archiving_finished"] = json::Number(tr_state.archiving_finished);
    message["archiving"] = json::Boolean(tr_state.archiving);
    message["reason"] = json::String(tr_state.reason);

    set_metadata(message, "job_metadata", tr_state.job_metadata);
    set_metadata(message, "file_metadata", tr_state.file_metadata);

    std::ostringstream stream;

    stream << "SS ";
    json::Writer::Write(message, stream);

    std::string msgStr = stream.str();
    int errCode = producer.runProducerMonitoring(msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendOptimizer(Producer &producer, const OptimizerInfo &opt_info)
{
    json::Object message;

    message["source_se"] = json::String(opt_info.source_se);
    message["dest_se"] = json::String(opt_info.dest_se);

    message["throughput"] = json::Number(opt_info.throughput);
    message["throughput_ema"] = json::Number(opt_info.ema);

    message["duration_avg"] = json::Number(opt_info.avgDuration);

    message["filesize_avg"] = json::Number(opt_info.filesizeAvg);
    message["filesize_stddev"] = json::Number(opt_info.filesizeStdDev);

    message["success_rate"] = json::Number(opt_info.successRate);
    message["retry_count"] = json::Number(opt_info.retryCount);

    message["active_count"] = json::Number(opt_info.activeCount);
    message["submitted_count"] = json::Number(opt_info.queueSize);

    message["connections"] = json::Number(opt_info.connections);
    message["diff"] = json::Number(opt_info.diff);

    message["rationale"] = json::String(opt_info.rationale);
    message["timestamp"] = json::Number(opt_info.timestamp);

    std::ostringstream stream;

    stream << "OP ";
    json::Writer::Write(message, stream);

    std::string msgStr = stream.str();
    int errCode = producer.runProducerMonitoring(msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}
