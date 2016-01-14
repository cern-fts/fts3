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


#include <iomanip>
#include <fstream>
#include "msg-ifce.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"

using fts3::config::ServerConfig;

bool msg_ifce::instanceFlag = false;
msg_ifce* msg_ifce::single = NULL;


msg_ifce *msg_ifce::getInstance()
{
    if (!instanceFlag) {
        single = new msg_ifce();
        instanceFlag = true;
        return single;
    }
    else {
        return single;
    }
}


msg_ifce::msg_ifce(): state(MSG_IFCE_WAITING_START)
{
}


msg_ifce::~msg_ifce()
{
    instanceFlag = false;
}


std::string msg_ifce::SendTransferStartMessage(Producer &producer, const transfer_completed &tr_started)
{
    if (state != MSG_IFCE_WAITING_START) {
        FTS3_COMMON_LOGGER_LOG(WARNING, "Trying to send a start message, but the internal state is not MSG_IFCE_WAITING_START");
        return std::string();
    }

    state = MSG_IFCE_WAITING_FINISH;

    std::ostringstream msg;
        
    msg << "ST {";
    msg << "\"agent_fqdn\":\"";
    msg << tr_started.agent_fqdn;
    msg << "\"";

    msg << ",\"transfer_id\":\"";
    msg << tr_started.transfer_id;
    msg << "\"";

    msg << ",\"endpnt\":\"";
    msg << tr_started.endpoint;
    msg << "\"";

    msg << ",\"timestamp\":\"";
    msg << getTimestampStr();
    msg << "\"";

    msg << ",\"src_srm_v\":\"";
    msg << tr_started.source_srm_version;
    msg << "\"";

    msg << ",\"dest_srm_v\":\"";
    msg << tr_started.destination_srm_version;
    msg << "\"";

    msg << ",\"vo\":\"";
    msg << tr_started.vo;
    msg << "\"";

    msg << ",\"src_url\":\"";
    msg << tr_started.source_url;
    msg << "\"";

    msg << ",\"dst_url\":\"";
    msg << tr_started.dest_url;
    msg << "\"";

    msg << ",\"src_hostname\":\"";
    msg << tr_started.source_hostname;
    msg << "\"";

    msg << ",\"dst_hostname\":\"";
    msg << tr_started.dest_hostname;
    msg << "\"";

    msg << ",\"src_site_name\":\"";
    msg << tr_started.source_site_name;
    msg << "\"";

    msg << ",\"dst_site_name\":\"";
    msg << tr_started.dest_site_name;
    msg << "\"";

    msg << ",\"t_channel\":\"";
    msg << tr_started.t_channel;
    msg << "\"";

    msg << ",\"srm_space_token_src\":\"";
    msg << tr_started.srm_space_token_source;
    msg << "\"";

    msg << ",\"srm_space_token_dst\":\"";
    msg << tr_started.srm_space_token_dest;
    msg << "\"";


    msg << ",\"user_dn\":\"";
    msg << tr_started.user_dn;
    msg << "\"";

    if (tr_started.file_metadata.length() > 0) {
        if (tr_started.file_metadata == "x") {
            msg << ",\"file_metadata\":\"\"";
        }
        else {
            msg << ",\"file_metadata\":";
            msg << tr_started.file_metadata;
            msg << "";
        }
    }
    else {
        msg << ",\"file_metadata\":\"\"";
    }

    if (tr_started.job_metadata.length() > 0) {
        msg << ",\"job_metadata\":";
        msg << tr_started.job_metadata;
        msg << "";
    }
    else {
        msg << ",\"job_metadata\":\"\"";
    }

    msg << "}";

    std::string msgStr = msg.str();
    int errCode = restoreMessageToDisk(producer, msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}


std::string msg_ifce::SendTransferFinishMessage(Producer &producer, const transfer_completed &tr_completed, bool force)
{
    if (!force && state != MSG_IFCE_WAITING_FINISH) {
        FTS3_COMMON_LOGGER_LOG(WARNING, "Trying to send a finish message, but the internal state is not MSG_IFCE_WAITING_FINISH");
        return std::string();
    }

    state = MSG_IFCE_WAITING_START;

    std::ostringstream msg;

    msg << "CO {";

    msg << "\"tr_id\":\"";
    msg << tr_completed.transfer_id;
    msg << "\"";

    msg << ",\"endpnt\":\"";
    msg << tr_completed.endpoint;
    msg << "\"";

    msg << ",\"src_srm_v\":\"";
    msg << tr_completed.source_srm_version;
    msg << "\"";

    msg << ",\"dest_srm_v\":\"";
    msg << tr_completed.destination_srm_version;
    msg << "\"";

    msg << ",\"vo\":\"";
    msg << tr_completed.vo;
    msg << "\"";

    msg << ",\"src_url\":\"";
    msg << tr_completed.source_url;
    msg << "\"";

    msg << ",\"dst_url\":\"";
    msg << tr_completed.dest_url;
    msg << "\"";

    msg << ",\"src_hostname\":\"";
    msg << tr_completed.source_hostname;
    msg << "\"";

    msg << ",\"dst_hostname\":\"";
    msg << tr_completed.dest_hostname;
    msg << "\"";

    msg << ",\"src_site_name\":\"";
    msg << tr_completed.source_site_name;
    msg << "\"";

    msg << ",\"dst_site_name\":\"";
    msg << tr_completed.dest_site_name;
    msg << "\"";

    msg << ",\"t_channel\":\"";
    msg << tr_completed.t_channel;
    msg << "\"";

    msg << ",\"timestamp_tr_st\":\"";
    msg << tr_completed.timestamp_transfer_started;
    msg << "\"";

    msg << ",\"timestamp_tr_comp\":\"";
    msg << tr_completed.timestamp_transfer_completed;
    msg << "\"";

    msg << ",\"timestamp_chk_src_st\":\"";
    msg << tr_completed.timestamp_checksum_source_started;
    msg << "\"";

    msg << ",\"timestamp_chk_src_ended\":\"";
    msg << tr_completed.timestamp_checksum_source_ended;
    msg << "\"";

    msg << ",\"timestamp_checksum_dest_st\":\"";
    msg << tr_completed.timestamp_checksum_dest_started;
    msg << "\"";

    msg << ",\"timestamp_checksum_dest_ended\":\"";
    msg << tr_completed.timestamp_checksum_dest_ended;
    msg << "\"";

    msg << ",\"t_timeout\":\"";
    msg << tr_completed.transfer_timeout;
    msg << "\"";

    msg << ",\"chk_timeout\":\"";
    msg << tr_completed.checksum_timeout;
    msg << "\"";

    msg << ",\"t_error_code\":\"";
    msg << tr_completed.transfer_error_code;
    msg << "\"";

    msg << ",\"tr_error_scope\":\"";
    msg << tr_completed.transfer_error_scope;
    msg << "\"";

    msg << ",\"t_failure_phase\":\"";
    msg << tr_completed.failure_phase;
    msg << "\"";

    msg << ",\"tr_error_category\":\"";
    msg << tr_completed.transfer_error_category;
    msg << "\"";

    msg << ",\"t_final_transfer_state\":\"";
    msg << tr_completed.final_transfer_state;
    msg << "\"";

    msg << ",\"tr_bt_transfered\":\"";
    msg << tr_completed.total_bytes_transfered;
    msg << "\"";

    msg << ",\"nstreams\":\"";
    msg << tr_completed.number_of_streams;
    msg << "\"";

    msg << ",\"buf_size\":\"";
    msg << tr_completed.tcp_buffer_size;
    msg << "\"";

    msg << ",\"tcp_buf_size\":\"";
    msg << tr_completed.tcp_buffer_size;
    msg << "\"";

    msg << ",\"block_size\":\"";
    msg << tr_completed.block_size;
    msg << "\"";

    msg << ",\"f_size\":\"";
    msg << tr_completed.file_size;
    msg << "\"";

    msg << ",\"time_srm_prep_st\":\"";
    msg << tr_completed.time_spent_in_srm_preparation_start;
    msg << "\"";

    msg << ",\"time_srm_prep_end\":\"";
    msg << tr_completed.time_spent_in_srm_preparation_end;
    msg << "\"";

    msg << ",\"time_srm_fin_st\":\"";
    msg << tr_completed.time_spent_in_srm_finalization_start;
    msg << "\"";

    msg << ",\"time_srm_fin_end\":\"";
    msg << tr_completed.time_spent_in_srm_finalization_end;
    msg << "\"";

    msg << ",\"srm_space_token_src\":\"";
    msg << tr_completed.srm_space_token_source;
    msg << "\"";

    msg << ",\"srm_space_token_dst\":\"";
    msg << tr_completed.srm_space_token_dest;
    msg << "\"";

    msg << ",\"t__error_message\":\"";

    std::string temp = ReplaceNonPrintableCharacters(tr_completed.transfer_error_message);
    temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\''), temp.end());
    temp.erase(std::remove(temp.begin(), temp.end(), '\"'), temp.end());

    if (temp.length() > 1024) {
        temp.erase(1024);
    }

    msg << temp;

    msg << "\"";

    msg << ",\"tr_timestamp_start\":\"";
    msg << tr_completed.tr_timestamp_start;
    msg << "\"";

    if (tr_completed.tr_timestamp_complete.empty() || tr_completed.tr_timestamp_complete.length() == 0) {
        msg << ",\"tr_timestamp_complete\":\"";
        msg << getTimestampStr();
        msg << "\"";
    }
    else {
        msg << ",\"tr_timestamp_complete\":\"";
        msg << tr_completed.tr_timestamp_complete;
        msg << "\"";
    }

    msg << ",\"channel_type\":\"";
    msg << tr_completed.channel_type;
    msg << "\"";

    msg << ",\"user_dn\":\"";
    msg << tr_completed.user_dn;
    msg << "\"";

    if (tr_completed.file_metadata.length() > 0) {
        if (tr_completed.file_metadata == "x") {
            msg << ",\"file_metadata\":\"\"";
        }
        else {
            msg << ",\"file_metadata\":";
            msg << tr_completed.file_metadata;
            msg << "";
        }
    }
    else {
        msg << ",\"file_metadata\":\"\"";
    }

    if (tr_completed.job_metadata.length() > 0) {
        msg << ",\"job_metadata\":";
        msg << tr_completed.job_metadata;
        msg << "";
    }
    else {
        msg << ",\"job_metadata\":\"\"";
    }

    msg << ",\"retry\":\"";
    msg << tr_completed.retry;
    msg << "\"";

    msg << ",\"retry_max\":\"";
    msg << tr_completed.retry_max;
    msg << "\"";

    msg << ",\"job_m_replica\":\"";
    msg << tr_completed.job_m_replica;
    msg << "\"";

    msg << ",\"job_state\":\"";
    msg << tr_completed.job_state;
    msg << "\"";

    msg << ",\"is_recoverable\":\"";
    msg << tr_completed.is_recoverable;
    msg << "\"";

    msg << "}";

    std::string msgStr = msg.str();
    int errCode = restoreMessageToDisk(producer, msgStr);
    if (errCode == 0) {
        return msgStr;
    }
    else {
        char buffer[512];
        return strerror_r(errCode, buffer, sizeof(buffer));
    }
}
