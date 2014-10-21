/**
 *  Copyright (c) Members of the EGEE Collaboration. 2004.
 *  See http://www.eu-egee.org/partners/ for details on the copyright
 *  holders.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */


#ifndef MSG_IFCE_C

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <fstream>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "msg-ifce.h"
#include "utility_routines.h"
#include "Logger.h"




bool msg_ifce::instanceFlag = false;
msg_ifce* msg_ifce::single = NULL;

msg_ifce* msg_ifce::getInstance()
{
    if (!instanceFlag)
        {
            single = new msg_ifce();
            instanceFlag = true;
            return single;
        }
    else
        {
            return single;
        }
}


msg_ifce::msg_ifce() /*private constructor*/
{
    read_initial_config();
}

msg_ifce::~msg_ifce()
{
    instanceFlag = false;
}


void msg_ifce::SendTransferStartMessage(transfer_completed *tr_started)
{

    if(false == getACTIVE())
        return;
    string text("");
    try
        {

            // Create a messages
            text = "ST {";
            text.append("\"$a$\":\"");
            text.append(tr_started->agent_fqdn);
            text += "\"";

            text.append(",\"$b$\":\"");
            text.append(tr_started->transfer_id);
            text.append("\"");

            text.append(",\"$c$\":\"");
            text.append(tr_started->endpoint);
            text.append("\"");

            text.append(",\"$d$\":\"");
            text.append(getTimestamp());
            text.append("\"");

            text.append(",\"$e$\":\"");
            text.append(tr_started->source_srm_version);
            text.append("\"");

            text.append(",\"$f$\":\"");
            text.append(tr_started->destination_srm_version);
            text.append("\"");

            text.append(",\"$g$\":\"");
            text.append(tr_started->vo);
            text.append("\"");

            text.append(",\"$h$\":\"");
            text.append(tr_started->source_url);
            text.append("\"");

            text.append(",\"$i$\":\"");
            text.append(tr_started->dest_url);
            text.append("\"");

            text.append(",\"$j$\":\"");
            text.append(tr_started->source_hostname);
            text.append("\"");

            text.append(",\"$k$\":\"");
            text.append(tr_started->dest_hostname);
            text.append("\"");

            text.append(",\"$l$\":\"");
            text.append(tr_started->source_site_name);
            text.append("\"");

            text.append(",\"$m$\":\"");
            text.append(tr_started->dest_site_name);
            text.append("\"");

            text.append(",\"$n$\":\"");
            text.append(tr_started->t_channel);
            text.append("\"");

            text.append(",\"$o$\":\"");
            text.append(tr_started->srm_space_token_source);
            text.append("\"");

            text.append(",\"$p$\":\"");
            text.append(tr_started->srm_space_token_dest);
            text.append("\"");


            text.append(",\"$q$\":\"");
            text.append(tr_started->user_dn);
            text.append("\"");

            if(tr_started->file_metadata.length() > 0)
                {
                    text.append(",\"$r$\":");
                    text.append(tr_started->file_metadata);
                    text.append("");
                }
            else
                {
                    text.append(",\"$r$\":\"\"");
                }

            if(tr_started->job_metadata.length() > 0)
                {
                    text.append(",\"$s$\":");
                    text.append(tr_started->job_metadata);
                    text.append("");
                }
            else
                {
                    text.append(",\"$s$\":\"\"");
                }


            text.append("}");

            send_message(text);
        }
    catch (...)
        {
            send_message(text);
            errorMessage = "Cannot send transfer started message with ID: " + tr_started->transfer_id ;
            logger::writeLog(errorMessage);
        }
}

void msg_ifce::SendTransferFinishMessage(transfer_completed *tr_completed)
{

    if(false == getACTIVE())
        return;

    string text("");
    try
        {

            // Create a messages
            text = string("CO {");
            text.append("\"$a$\":\"");
            text.append(tr_completed->transfer_id);
            text.append("\"");

            text.append(",\"$b$\":\"");
            text.append(tr_completed->endpoint);
            text.append("\"");

            text.append(",\"$c$\":\"");
            text.append(tr_completed->source_srm_version);
            text.append("\"");

            text.append(",\"$d$\":\"");
            text.append(tr_completed->destination_srm_version);
            text.append("\"");

            text.append(",\"$e$\":\"");
            text.append(tr_completed->vo);
            text.append("\"");

            text.append(",\"$f$\":\"");
            text.append(tr_completed->source_url);
            text.append("\"");

            text.append(",\"$g$\":\"");
            text.append(tr_completed->dest_url);
            text.append("\"");

            text.append(",\"$h$\":\"");
            text.append(tr_completed->source_hostname);
            text.append("\"");

            text.append(",\"$i$\":\"");
            text.append(tr_completed->dest_hostname);
            text.append("\"");

            text.append(",\"$j$\":\"");
            text.append(tr_completed->source_site_name);
            text.append("\"");

            text.append(",\"$k$\":\"");
            text.append(tr_completed->dest_site_name);
            text.append("\"");

            text.append(",\"$l$\":\"");
            text.append(tr_completed->t_channel);
            text.append("\"");

            text.append(",\"$m$\":\"");
            text.append(tr_completed->timestamp_transfer_started);
            text.append("\"");

            text.append(",\"$n$\":\"");
            text.append(tr_completed->timestamp_transfer_completed);
            text.append("\"");

            text.append(",\"$o$\":\"");
            text.append(tr_completed->timestamp_checksum_source_started);
            text.append("\"");

            text.append(",\"$p$\":\"");
            text.append(tr_completed->timestamp_checksum_source_ended);
            text.append("\"");

            text.append(",\"$q$\":\"");
            text.append(tr_completed->timestamp_checksum_dest_started);
            text.append("\"");

            text.append(",\"$r$\":\"");
            text.append(tr_completed->timestamp_checksum_dest_ended);
            text.append("\"");

            text.append(",\"$s$\":\"");
            text.append(tr_completed->transfer_timeout);
            text.append("\"");

            text.append(",\"$t$\":\"");
            text.append(tr_completed->checksum_timeout);
            text.append("\"");

            text.append(",\"$u$\":\"");
            text.append(tr_completed->transfer_error_code);
            text.append("\"");

            text.append(",\"$v$\":\"");
            text.append(tr_completed->transfer_error_scope);
            text.append("\"");

            text.append(",\"$w$\":\"");
            text.append(tr_completed->failure_phase);
            text.append("\"");

            text.append(",\"$x$\":\"");
            text.append(tr_completed->transfer_error_category);
            text.append("\"");

            text.append(",\"$y$\":\"");
            text.append(tr_completed->final_transfer_state);
            text.append("\"");

            text.append(",\"$z$\":\"");
            text.append(tr_completed->total_bytes_transfered);
            text.append("\"");

            text.append(",\"$0$\":\"");
            text.append(tr_completed->number_of_streams);
            text.append("\"");

            text.append(",\"$1$\":\"");
            text.append(tr_completed->tcp_buffer_size);
            text.append("\"");

            text.append(",\"$2$\":\"");
            text.append(tr_completed->tcp_buffer_size);
            text.append("\"");

            text.append(",\"$3$\":\"");
            text.append(tr_completed->block_size);
            text.append("\"");

            text.append(",\"$4$\":\"");
            text.append(tr_completed->file_size);
            text.append("\"");

            text.append(",\"$5$\":\"");
            text.append(tr_completed->time_spent_in_srm_preparation_start);
            text.append("\"");

            text.append(",\"$6$\":\"");
            text.append(tr_completed->time_spent_in_srm_preparation_end);
            text.append("\"");

            text.append(",\"$7$\":\"");
            text.append(tr_completed->time_spent_in_srm_finalization_start);
            text.append("\"");

            text.append(",\"$8$\":\"");
            text.append(tr_completed->time_spent_in_srm_finalization_end);
            text.append("\"");

            text.append(",\"$9$\":\"");
            text.append(tr_completed->srm_space_token_source);
            text.append("\"");

            text.append(",\"$10$\":\"");
            text.append(tr_completed->srm_space_token_dest);
            text.append("\"");

            text.append(",\"$11$\":\"");
            string temp = ReplaceNonPrintableCharacters(tr_completed->transfer_error_message);
            temp.erase(std::remove(temp.begin(), temp.end(), '\n'), temp.end());
            temp.erase(std::remove(temp.begin(), temp.end(), '\''), temp.end());
            temp.erase(std::remove(temp.begin(), temp.end(), '\"'), temp.end());

            if(temp.length() > 1024)
                temp.erase(1024);

            text.append(temp);

            text.append("\"");

            text.append(",\"$12$\":\"");
            text.append(tr_completed->tr_timestamp_start);
            text.append("\"");

            text.append(",\"$13$\":\"");
            text.append(tr_completed->tr_timestamp_complete);
            text.append("\"");

            text.append(",\"$14$\":\"");
            text.append(tr_completed->channel_type);
            text.append("\"");

            text.append(",\"$15$\":\"");
            text.append(tr_completed->user_dn);
            text.append("\"");

            if(tr_completed->file_metadata.length() > 0)
                {
                    text.append(",\"$16$\":");
                    text.append(tr_completed->file_metadata);
                    text.append("");
                }
            else
                {
                    text.append(",\"$16$\":\"\"");
                }

            if(tr_completed->job_metadata.length() > 0)
                {
                    text.append(",\"$17$\":");
                    text.append(tr_completed->job_metadata);
                    text.append("");
                }
            else
                {
                    text.append(",\"$17$\":\"\"");
                }

            text.append(",\"$18$\":\"");
            text.append(tr_completed->retry);
            text.append("\"");

            text.append(",\"$19$\":\"");
            text.append(tr_completed->retry_max);
            text.append("\"");

            text.append(",\"$20$\":\"");
            text.append(tr_completed->job_m_replica);
            text.append("\"");

            text.append(",\"$21$\":\"");
            text.append(tr_completed->job_state);
            text.append("\"");


            text.append("}");

            send_message(text);
        }
    catch (...)
        {
            send_message(text);
            errorMessage = "Cannot send transfer completed message with ID: " + tr_completed->transfer_id ;
            logger::writeLog(errorMessage);
        }
}


bool msg_ifce::read_initial_config()
{
    try
        {
            bool fileIsOk = get_mon_cfg_file();
            if(!fileIsOk)
                {
                    logger::writeLog("Cannot read msg cfg file, check file name and path");
                    return false;
                }
        }
    catch(...)
        {
            logger::writeLog("Cannot read msg cfg file, check file name and path");
            return false;
        }

    return true;
}


/*setter and getter for the message itself
  transfer_completed setters
*/

void msg_ifce::set_agent_fqdn(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->agent_fqdn = value;
}

void msg_ifce::set_transfer_id(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        {
            if (value.length() > 0)
                tr_completed->transfer_id = value;
            else
                tr_completed->transfer_id = "";
        }
}

void msg_ifce::set_endpoint(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->endpoint = value;
}

void msg_ifce::set_source_srm_version(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->source_srm_version = value;
}

void msg_ifce::set_destination_srm_version(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->destination_srm_version = value;
}

void msg_ifce::set_vo(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->vo = value;
}

void msg_ifce::set_source_site_name(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->source_site_name = value;
}

void msg_ifce::set_dest_site_name(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->dest_site_name = value;
}


void msg_ifce::set_source_url(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->source_url = value;
}

void msg_ifce::set_dest_url(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->dest_url = value;
}

void msg_ifce::set_source_hostname(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->source_hostname = value;
}

void msg_ifce::set_dest_hostname(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        {
            tr_completed->dest_hostname = value;
        }
}


void msg_ifce::set_t_channel(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->t_channel = value;
}

void msg_ifce::set_timestamp_transfer_started(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->timestamp_transfer_started = value;
}

void msg_ifce::set_timestamp_transfer_completed(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        {
            tr_completed->timestamp_transfer_completed = value;
        }
}

void msg_ifce::set_timestamp_checksum_source_started(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->timestamp_checksum_source_started = value;
}

void msg_ifce::set_timestamp_checksum_source_ended(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->timestamp_checksum_source_ended = value;
}

void msg_ifce::set_timestamp_checksum_dest_started(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->timestamp_checksum_dest_started = value;
}

void msg_ifce::set_timestamp_checksum_dest_ended(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->timestamp_checksum_dest_ended = value;
}

void msg_ifce::set_transfer_timeout(transfer_completed* tr_completed, unsigned value)
{
    if (tr_completed)
        tr_completed->transfer_timeout = to_string<unsigned int>(value, std::dec);
}

void msg_ifce::set_checksum_timeout(transfer_completed* tr_completed, int value)
{
    if (tr_completed)
        tr_completed->checksum_timeout = to_string<unsigned int>(value, std::dec);
}

void msg_ifce::set_transfer_error_code(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->transfer_error_code = value;
}

void msg_ifce::set_transfer_error_scope(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed && tr_completed->transfer_error_scope.length() == 0)
        tr_completed->transfer_error_scope = value;
}

void msg_ifce::set_transfer_error_category(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed && tr_completed->transfer_error_category.length() == 0)
        tr_completed->transfer_error_category = value;
}

void msg_ifce::set_transfer_error_message(transfer_completed* tr_completed, const std::string & value)
{

    if (tr_completed && tr_completed->transfer_error_message.length() == 0)
        {
            tr_completed->transfer_error_message = value;
            set_transfer_error_code(tr_completed, extractNumber(value));
        }
}

void msg_ifce::set_failure_phase(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed && tr_completed->failure_phase.length() == 0)
        tr_completed->failure_phase = value;
}

void msg_ifce::set_final_transfer_state(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->final_transfer_state = value;
}

void msg_ifce::set_total_bytes_transfered(transfer_completed* tr_completed, off_t value)
{
    if (tr_completed)
        tr_completed->total_bytes_transfered = to_string<off_t>(value, std::dec);
}

void msg_ifce::set_number_of_streams(transfer_completed* tr_completed, unsigned value)
{
    if (tr_completed)
        tr_completed->number_of_streams = to_string<unsigned int>(value, std::dec);
}

void msg_ifce::set_tcp_buffer_size(transfer_completed* tr_completed, unsigned value)
{
    if (tr_completed)
        tr_completed->tcp_buffer_size = to_string<unsigned int>(value, std::dec);
}

void msg_ifce::set_block_size(transfer_completed* tr_completed, unsigned value)
{
    if (tr_completed)
        tr_completed->block_size = to_string<unsigned int>(value, std::dec);
}

void msg_ifce::set_file_size(transfer_completed* tr_completed, off_t value)
{
    if (tr_completed)
        tr_completed->file_size = to_string<off_t>(value, std::dec);
}

void msg_ifce::set_time_spent_in_srm_preparation_start(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->time_spent_in_srm_preparation_start = value;
}

void msg_ifce::set_time_spent_in_srm_preparation_end(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->time_spent_in_srm_preparation_end = value;
}

void msg_ifce::set_time_spent_in_srm_finalization_start(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->time_spent_in_srm_finalization_start = value;
}

void msg_ifce::set_time_spent_in_srm_finalization_end(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->time_spent_in_srm_finalization_end = value;
}

void msg_ifce::set_srm_space_token_source(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->srm_space_token_source = value;
}

void msg_ifce::set_srm_space_token_dest(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->srm_space_token_dest = value;
}

void msg_ifce::set_tr_timestamp_start(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->tr_timestamp_start = value;
}
void msg_ifce::set_tr_timestamp_complete(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->tr_timestamp_complete = value;
}

void msg_ifce::set_channel_type(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->channel_type = value;
}

void msg_ifce::set_user_dn(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->user_dn = value;
}

void msg_ifce::set_file_metadata(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->file_metadata = value;
}


void msg_ifce::set_job_metadata(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->job_metadata = value;
}


void msg_ifce::set_retry(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->retry = value;
}


void msg_ifce::set_retry_max(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->retry_max = value;
}

std::string msg_ifce::getTimestamp()
{
    return _getTimestamp();
}

void msg_ifce::set_job_state(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->job_state = value;
}

void msg_ifce::set_job_m_replica(transfer_completed* tr_completed, const std::string & value)
{
    if (tr_completed)
        tr_completed->job_m_replica = value;
}


#endif
