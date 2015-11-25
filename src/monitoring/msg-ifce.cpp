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
#include "UtilityRoutines.h"
#include "common/Logger.h"


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

/*private constructor*/
msg_ifce::msg_ifce(): state(MSG_IFCE_WAITING_START)
{
    read_initial_config();
}

msg_ifce::~msg_ifce()
{
    instanceFlag = false;
}


std::string msg_ifce::SendTransferStartMessage(const transfer_completed &tr_started)
{
    std::string message;

    if (state != MSG_IFCE_WAITING_START) {
        FTS3_COMMON_LOGGER_LOG(WARNING, "Trying to send a start message, but the internal state is not MSG_IFCE_WAITING_START");
        return message;
    }

    state = MSG_IFCE_WAITING_FINISH;

    if(false == getACTIVE())
        return message;

    std::string text;
    try {
        std::ostringstream msg;
        
        msg << "ST {";
        msg << "\"$a$\":\"";
        msg << tr_started.agent_fqdn;
        msg << "\"";

        msg << ",\"$b$\":\"";
        msg << tr_started.transfer_id;
        msg << "\"";

        msg << ",\"$c$\":\"";
        msg << tr_started.endpoint;
        msg << "\"";

        msg << ",\"$d$\":\"";
        msg << getTimestampStr();
        msg << "\"";

        msg << ",\"$e$\":\"";
        msg << tr_started.source_srm_version;
        msg << "\"";

        msg << ",\"$f$\":\"";
        msg << tr_started.destination_srm_version;
        msg << "\"";

        msg << ",\"$g$\":\"";
        msg << tr_started.vo;
        msg << "\"";

        msg << ",\"$h$\":\"";
        msg << tr_started.source_url;
        msg << "\"";

        msg << ",\"$i$\":\"";
        msg << tr_started.dest_url;
        msg << "\"";

        msg << ",\"$j$\":\"";
        msg << tr_started.source_hostname;
        msg << "\"";

        msg << ",\"$k$\":\"";
        msg << tr_started.dest_hostname;
        msg << "\"";

        msg << ",\"$l$\":\"";
        msg << tr_started.source_site_name;
        msg << "\"";

        msg << ",\"$m$\":\"";
        msg << tr_started.dest_site_name;
        msg << "\"";

        msg << ",\"$n$\":\"";
        msg << tr_started.t_channel;
        msg << "\"";

        msg << ",\"$o$\":\"";
        msg << tr_started.srm_space_token_source;
        msg << "\"";

        msg << ",\"$p$\":\"";
        msg << tr_started.srm_space_token_dest;
        msg << "\"";


        msg << ",\"$q$\":\"";
        msg << tr_started.user_dn;
        msg << "\"";

        if (tr_started.file_metadata.length() > 0) {
            if (tr_started.file_metadata == "x") {
                msg << ",\"$r$\":\"\"";
            }
            else {
                msg << ",\"$r$\":";
                msg << tr_started.file_metadata;
                msg << "";
            }
        }
        else {
            msg << ",\"$r$\":\"\"";
        }

        if (tr_started.job_metadata.length() > 0) {
            msg << ",\"$s$\":";
            msg << tr_started.job_metadata;
            msg << "";
        }
        else {
            msg << ",\"$s$\":\"\"";
        }

        msg << "}";

        text = msg.str();
        message = restoreMessageToDisk(text);
        if (message.empty())
            return text;
        else
            return message;
    }
    catch (...) {
        //try again
        message = restoreMessageToDisk(text);
        if (message.empty())
            return text;
        else
            return message;
    }
}


std::string msg_ifce::SendTransferFinishMessage(const transfer_completed &tr_completed, bool force)
{
    std::string message;

    if (!force && state != MSG_IFCE_WAITING_FINISH) {
        FTS3_COMMON_LOGGER_LOG(WARNING, "Trying to send a finish message, but the internal state is not MSG_IFCE_WAITING_FINISH");
        return message;
    }

    state = MSG_IFCE_WAITING_START;

    if(false == getACTIVE())
        return message;

    std::string text("");

    try {
        std::ostringstream msg;

        msg << "CO {";

        msg << "\"$a$\":\"";
        msg << tr_completed.transfer_id;
        msg << "\"";

        msg << ",\"$b$\":\"";
        msg << tr_completed.endpoint;
        msg << "\"";

        msg << ",\"$c$\":\"";
        msg << tr_completed.source_srm_version;
        msg << "\"";

        msg << ",\"$d$\":\"";
        msg << tr_completed.destination_srm_version;
        msg << "\"";

        msg << ",\"$e$\":\"";
        msg << tr_completed.vo;
        msg << "\"";

        msg << ",\"$f$\":\"";
        msg << tr_completed.source_url;
        msg << "\"";

        msg << ",\"$g$\":\"";
        msg << tr_completed.dest_url;
        msg << "\"";

        msg << ",\"$h$\":\"";
        msg << tr_completed.source_hostname;
        msg << "\"";

        msg << ",\"$i$\":\"";
        msg << tr_completed.dest_hostname;
        msg << "\"";

        msg << ",\"$j$\":\"";
        msg << tr_completed.source_site_name;
        msg << "\"";

        msg << ",\"$k$\":\"";
        msg << tr_completed.dest_site_name;
        msg << "\"";

        msg << ",\"$l$\":\"";
        msg << tr_completed.t_channel;
        msg << "\"";

        msg << ",\"$m$\":\"";
        msg << tr_completed.timestamp_transfer_started;
        msg << "\"";

        msg << ",\"$n$\":\"";
        msg << tr_completed.timestamp_transfer_completed;
        msg << "\"";

        msg << ",\"$o$\":\"";
        msg << tr_completed.timestamp_checksum_source_started;
        msg << "\"";

        msg << ",\"$p$\":\"";
        msg << tr_completed.timestamp_checksum_source_ended;
        msg << "\"";

        msg << ",\"$q$\":\"";
        msg << tr_completed.timestamp_checksum_dest_started;
        msg << "\"";

        msg << ",\"$r$\":\"";
        msg << tr_completed.timestamp_checksum_dest_ended;
        msg << "\"";

        msg << ",\"$s$\":\"";
        msg << tr_completed.transfer_timeout;
        msg << "\"";

        msg << ",\"$t$\":\"";
        msg << tr_completed.checksum_timeout;
        msg << "\"";

        msg << ",\"$u$\":\"";
        msg << tr_completed.transfer_error_code;
        msg << "\"";

        msg << ",\"$v$\":\"";
        msg << tr_completed.transfer_error_scope;
        msg << "\"";

        msg << ",\"$w$\":\"";
        msg << tr_completed.failure_phase;
        msg << "\"";

        msg << ",\"$x$\":\"";
        msg << tr_completed.transfer_error_category;
        msg << "\"";

        msg << ",\"$y$\":\"";
        msg << tr_completed.final_transfer_state;
        msg << "\"";

        msg << ",\"$z$\":\"";
        msg << tr_completed.total_bytes_transfered;
        msg << "\"";

        msg << ",\"$0$\":\"";
        msg << tr_completed.number_of_streams;
        msg << "\"";

        msg << ",\"$1$\":\"";
        msg << tr_completed.tcp_buffer_size;
        msg << "\"";

        msg << ",\"$2$\":\"";
        msg << tr_completed.tcp_buffer_size;
        msg << "\"";

        msg << ",\"$3$\":\"";
        msg << tr_completed.block_size;
        msg << "\"";

        msg << ",\"$4$\":\"";
        msg << tr_completed.file_size;
        msg << "\"";

        msg << ",\"$5$\":\"";
        msg << tr_completed.time_spent_in_srm_preparation_start;
        msg << "\"";

        msg << ",\"$6$\":\"";
        msg << tr_completed.time_spent_in_srm_preparation_end;
        msg << "\"";

        msg << ",\"$7$\":\"";
        msg << tr_completed.time_spent_in_srm_finalization_start;
        msg << "\"";

        msg << ",\"$8$\":\"";
        msg << tr_completed.time_spent_in_srm_finalization_end;
        msg << "\"";

        msg << ",\"$9$\":\"";
        msg << tr_completed.srm_space_token_source;
        msg << "\"";

        msg << ",\"$10$\":\"";
        msg << tr_completed.srm_space_token_dest;
        msg << "\"";

        msg << ",\"$11$\":\"";

        std::string temp = ReplaceNonPrintableCharacters(tr_completed.transfer_error_message);
        temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
        temp.erase(std::remove(temp.begin(), temp.end(), '\''), temp.end());
        temp.erase(std::remove(temp.begin(), temp.end(), '\"'), temp.end());

        if (temp.length() > 1024) {
            temp.erase(1024);
        }

        msg << temp;

        msg << "\"";

        msg << ",\"$12$\":\"";
        msg << tr_completed.tr_timestamp_start;
        msg << "\"";

        if (tr_completed.tr_timestamp_complete.empty() || tr_completed.tr_timestamp_complete.length() == 0) {
            msg << ",\"$13$\":\"";
            msg << getTimestampStr();
            msg << "\"";
        }
        else {
            msg << ",\"$13$\":\"";
            msg << tr_completed.tr_timestamp_complete;
            msg << "\"";
        }

        msg << ",\"$14$\":\"";
        msg << tr_completed.channel_type;
        msg << "\"";

        msg << ",\"$15$\":\"";
        msg << tr_completed.user_dn;
        msg << "\"";

        if (tr_completed.file_metadata.length() > 0) {
            if (tr_completed.file_metadata == "x") {
                msg << ",\"$16$\":\"\"";
            }
            else {
                msg << ",\"$16$\":";
                msg << tr_completed.file_metadata;
                msg << "";
            }
        }
        else {
            msg << ",\"$16$\":\"\"";
        }

        if (tr_completed.job_metadata.length() > 0) {
            msg << ",\"$17$\":";
            msg << tr_completed.job_metadata;
            msg << "";
        }
        else {
            msg << ",\"$17$\":\"\"";
        }

        msg << ",\"$18$\":\"";
        msg << tr_completed.retry;
        msg << "\"";

        msg << ",\"$19$\":\"";
        msg << tr_completed.retry_max;
        msg << "\"";

        msg << ",\"$20$\":\"";
        msg << tr_completed.job_m_replica;
        msg << "\"";

        msg << ",\"$21$\":\"";
        msg << tr_completed.job_state;
        msg << "\"";

        msg << ",\"$22$\":\"";
        msg << tr_completed.is_recoverable;
        msg << "\"";

        msg << "}";

        text = msg.str();
        message = restoreMessageToDisk(text);
        if (message.empty())
            return text;
        else
            return message;

    }
    catch (...) {
        //try again
        message = restoreMessageToDisk(text);
        if (message.empty())
            return text;
        else
            return message;
    }
}


bool msg_ifce::read_initial_config()
{
    try {
        bool fileIsOk = get_mon_cfg_file();
        if (!fileIsOk) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Cannot read msg cfg file, check file name and path");
            return false;
        }
    }
    catch (...) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "Cannot read msg cfg file, check file name and path");
        return false;
    }

    return true;
}
