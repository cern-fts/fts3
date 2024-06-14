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
#include <fstream>
#include <json/json.h>

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


static void set_metadata(Json::Value& json, const std::string& key, const std::string& value)
{
    if (!value.empty()) {
        try {
            Json::Value metadata;
            std::istringstream valueStream(value);
            valueStream >> metadata;
            json[key] = metadata;
            return;
        }
        catch (...) {
            // continue
        }
    }

    json[key] = value;
}


std::string MsgIfce::SendTransferStartMessage(Producer &producer, const TransferCompleted &tr_started)
{
    Json::Value message;

    message["transfer_id"] = tr_started.transfer_id;
    message["job_id"] = tr_started.job_id;
    message["file_id"] = (Json::Value::UInt64) tr_started.file_id;
    message["endpnt"] = tr_started.endpoint;
    message["timestamp"] = (Json::Value::UInt64) getTimestampMillisecs();
    message["src_srm_v"] = tr_started.source_srm_version;
    message["dest_srm_v"] = tr_started.destination_srm_version;
    message["vo"] = tr_started.vo;
    message["src_url"] = tr_started.source_url;
    message["dst_url"] = tr_started.dest_url;
    message["src_hostname"] = tr_started.source_hostname;
    message["dst_hostname"] = tr_started.dest_hostname;
    message["src_site_name"] = tr_started.source_site_name;
    message["dst_site_name"] = tr_started.dest_site_name;
    message["t_channel"] = tr_started.t_channel;
    message["srm_space_token_src"] = tr_started.srm_space_token_source;
    message["srm_space_token_dst"] = tr_started.srm_space_token_dest;
    message["user_dn"] = tr_started.user_dn;

    if (tr_started.file_metadata != "x") {
        set_metadata(message, "file_metadata", tr_started.file_metadata);
    }
    else {
        message["file_metadata"] = "";
    }

    set_metadata(message, "job_metadata", tr_started.job_metadata);

    std::ostringstream stream;
    stream << "ST " << message;
    std::string msgStr = stream.str();

    int errCode = producer.runProducerMonitoring(msgStr);

    if (errCode == 0) {
        return msgStr;
    } else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendTransferFinishMessage(Producer &producer, const TransferCompleted &tr_completed)
{
    Json::Value message;

    message["tr_id"] = tr_completed.transfer_id;
    message["job_id"] = tr_completed.job_id;
    message["file_id"] = (Json::Value::UInt64) tr_completed.file_id;
    message["endpnt"] = tr_completed.endpoint;
    message["src_srm_v"] = tr_completed.source_srm_version;
    message["dest_srm_v"] = tr_completed.destination_srm_version;
    message["vo"] = tr_completed.vo;
    message["src_url"] = tr_completed.source_url;
    message["dst_url"] = tr_completed.dest_url;
    message["src_hostname"] = tr_completed.source_hostname;
    message["dst_hostname"] = tr_completed.dest_hostname;
    message["src_se"] = tr_completed.source_se;
    message["dst_se"] = tr_completed.dest_se;
    message["protocol"] = tr_completed.protocol;
    message["src_site_name"] = tr_completed.source_site_name;
    message["dst_site_name"] = tr_completed.dest_site_name;
    message["t_channel"] = tr_completed.t_channel;
    message["timestamp_tr_st"] = (Json::Value::UInt64) tr_completed.timestamp_transfer_started;
    message["timestamp_tr_comp"] = (Json::Value::UInt64) tr_completed.timestamp_transfer_completed;
    message["timestamp_chk_src_st"] = (Json::Value::UInt64) tr_completed.timestamp_checksum_source_started;
    message["timestamp_chk_src_ended"] = (Json::Value::UInt64) tr_completed.timestamp_checksum_source_ended;
    message["timestamp_checksum_dest_st"] = (Json::Value::UInt64) tr_completed.timestamp_checksum_dest_started;
    message["timestamp_checksum_dest_ended"] = (Json::Value::UInt64) tr_completed.timestamp_checksum_dest_ended;
    message["t_timeout"] = (Json::Value::UInt64) tr_completed.transfer_timeout;
    message["chk_timeout"] = (Json::Value::Int) tr_completed.checksum_timeout;
    message["t_error_code"] = (Json::Value::Int) tr_completed.transfer_error_code;
    message["tr_error_scope"] = tr_completed.transfer_error_scope;
    message["t_failure_phase"] = tr_completed.failure_phase;
    message["tr_error_category"] = tr_completed.transfer_error_category;
    message["t_final_transfer_state"] = tr_completed.final_transfer_state;
    message["t_final_transfer_state_flag"] = (Json::Value::Int) tr_completed.final_transfer_state_flag;
    message["tr_bt_transfered"] = (Json::Value::UInt64) tr_completed.total_bytes_transferred;
    message["nstreams"] = (Json::Value::UInt) tr_completed.number_of_streams;
    message["buf_size"] = (Json::Value::UInt64) tr_completed.tcp_buffer_size;
    message["tcp_buf_size"] = (Json::Value::UInt64) tr_completed.tcp_buffer_size;
    message["block_size"] = (Json::Value::UInt) tr_completed.block_size;
    message["scitag"] = (Json::Value::UInt) tr_completed.scitag;
    // Prepare to drop "f_size" field in the future
    message["f_size"] = (Json::Value::Int64) tr_completed.file_size;
    message["file_size"] = (Json::Value::Int64) tr_completed.file_size;
    message["time_srm_prep_st"] = (Json::Value::UInt64) tr_completed.time_spent_in_srm_preparation_start;
    message["time_srm_prep_end"] = (Json::Value::UInt64) tr_completed.time_spent_in_srm_preparation_end;
    message["time_srm_fin_st"] = (Json::Value::UInt64) tr_completed.time_spent_in_srm_finalization_start;
    message["time_srm_fin_end"] = (Json::Value::UInt64) tr_completed.time_spent_in_srm_finalization_end;
    message["srm_space_token_src"] = tr_completed.srm_space_token_source;
    message["srm_space_token_dst"] = tr_completed.srm_space_token_dest;

    std::string temp = ReplaceNonPrintableCharacters(tr_completed.transfer_error_message);
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\''), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\"'), temp.end());
    if (temp.size() > 1024) {
        temp.erase(1024);
    }

    message["t__error_message"] = temp;
    message["tr_timestamp_start"] = (Json::Value::UInt64) tr_completed.tr_timestamp_start;

    if (tr_completed.tr_timestamp_complete) {
        message["tr_timestamp_complete"] = (Json::Value::UInt64) tr_completed.tr_timestamp_complete;
    } else {
        message["tr_timestamp_complete"] = (Json::Value::UInt64) getTimestampMillisecs();
    }

    message["transfer_time"] = (Json::Value::Int64) tr_completed.transfer_time_ms;
    message["operation_time"] = (Json::Value::Int64) tr_completed.operation_time_ms;
    message["throughput"] = tr_completed.throughput_bps;

    message["srm_preparation_time"] = (Json::Value::Int64) tr_completed.srm_preparation_time_ms;
    message["srm_finalization_time"] = (Json::Value::Int64) tr_completed.srm_finalization_time_ms;
    message["srm_overhead_time"] = (Json::Value::UInt64) tr_completed.srm_overhead_time_ms;
    message["srm_overhead_percentage"] = tr_completed.srm_overhead_percentage;

    message["timestamp_checksum_src_diff"] = (Json::Value::Int64) tr_completed.checksum_source_time_ms;
    message["timestamp_checksum_dst_diff"] = (Json::Value::Int64) tr_completed.checksum_dest_time_ms;

    message["channel_type"] = tr_completed.channel_type;
    message["user_dn"] = tr_completed.user_dn;

    if (tr_completed.file_metadata != "x") {
        set_metadata(message, "file_metadata", tr_completed.file_metadata);
    } else {
        message["file_metadata"] = "";
    }

    set_metadata(message, "job_metadata", tr_completed.job_metadata);
    message["activity"] = tr_completed.activity;
    message["retry"] = (Json::Value::UInt) tr_completed.retry;
    message["retry_max"] = (Json::Value::UInt) tr_completed.retry_max;
    message["job_m_replica"] = tr_completed.job_m_replica;
    message["job_multihop"] = tr_completed.job_multihop;
    message["transfer_lasthop"] = tr_completed.is_lasthop;
    message["is_archiving"] = tr_completed.is_archiving;
    message["job_state"] = tr_completed.job_state;
    message["is_recoverable"] = tr_completed.is_recoverable;
    message["ipv6"] = tr_completed.ipv6;
    message["ipver"] = tr_completed.ipver;
    message["eviction_code"] = (Json::Value::Int) tr_completed.eviction_code;
    message["cleanup_code"] = (Json::Value::Int) tr_completed.cleanup_code;
    message["final_destination"] = tr_completed.final_destination;
    message["transfer_type"] = tr_completed.transfer_type;
    message["auth_method"] = tr_completed.auth_method;
    message["overwrite_on_disk_flag"] = tr_completed.overwrite_on_disk_flag;
    message["overwrite_on_disk_deletion_code"] = (Json::Value::Int) tr_completed.overwrite_on_disk_deletion_code;

    std::ostringstream stream;
    stream << "CO " << message;
    std::string msgStr = stream.str();

    int errCode = producer.runProducerMonitoring(msgStr);

    if (errCode == 0) {
        return msgStr;
    } else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendTransferStatusChange(Producer &producer, const TransferState &tr_state)
{
    Json::Value message;

    message["user_dn"] = tr_state.user_dn;
    message["src_url"] = tr_state.source_url;
    message["dst_url"] = tr_state.dest_url;
    message["vo_name"] = tr_state.vo_name;
    message["source_se"] = tr_state.source_se;
    message["dest_se"] = tr_state.dest_se;
    message["job_id"] = tr_state.job_id;
    message["file_id"] = (Json::Value::UInt64) tr_state.file_id;
    message["job_state"] = tr_state.job_state;
    message["file_state"] = tr_state.file_state;
    message["retry_counter"] = (Json::Value::Int) tr_state.retry_counter;
    message["retry_max"] = (Json::Value::Int) tr_state.retry_max;
    message["user_filesize"] = (Json::Value::Int64) tr_state.user_filesize;
    message["timestamp"] = (Json::Value::UInt64) tr_state.timestamp;
    message["submit_time"] = (Json::Value::UInt64) tr_state.submit_time;
    message["staging_start"] = (Json::Value::UInt64) tr_state.staging_start;
    message["staging_finished"] = (Json::Value::UInt64) tr_state.staging_finished;
    message["staging"] = tr_state.staging;
    message["archiving_start"] = (Json::Value::UInt64) tr_state.archiving_start;
    message["archiving_finished"] = (Json::Value::UInt64) tr_state.archiving_finished;
    message["archiving"] = tr_state.archiving;
    message["reason"] = tr_state.reason;

    set_metadata(message, "job_metadata", tr_state.job_metadata);
    set_metadata(message, "file_metadata", tr_state.file_metadata);

    std::ostringstream stream;
    stream << "SS " << message;
    std::string msgStr = stream.str();

    int errCode = producer.runProducerMonitoring(msgStr);

    if (errCode == 0) {
        return msgStr;
    } else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string MsgIfce::SendOptimizer(Producer &producer, const OptimizerInfo &opt_info)
{
    Json::Value message;

    message["source_se"] = opt_info.source_se;
    message["dest_se"] = opt_info.dest_se;

    message["throughput"] = opt_info.throughput;
    message["throughput_ema"] = opt_info.ema;

    message["duration_avg"] = (Json::Value::Int64) opt_info.avgDuration;

    message["filesize_avg"] = opt_info.filesizeAvg;
    message["filesize_stddev"] = opt_info.filesizeStdDev;

    message["success_rate"] = opt_info.successRate;
    message["retry_count"] = (Json::Value::Int) opt_info.retryCount;

    message["active_count"] = (Json::Value::Int) opt_info.activeCount;
    message["submitted_count"] = (Json::Value::Int) opt_info.queueSize;

    message["connections"] = (Json::Value::Int) opt_info.connections;
    message["diff"] = (Json::Value::Int) opt_info.diff;

    message["rationale"] = opt_info.rationale;
    message["timestamp"] = (Json::Value::Int64) opt_info.timestamp;

    std::ostringstream stream;

    stream << "OP " << message;
    std::string msgStr = stream.str();

    int errCode = producer.runProducerMonitoring(msgStr);

    if (errCode == 0) {
        return msgStr;
    } else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}
