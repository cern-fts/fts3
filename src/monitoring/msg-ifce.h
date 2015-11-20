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

#ifndef MSG_IFCE_H

#include <iostream>
#include <vector>
#include <sstream>

/**
 * This is the external interface of the FTS messaging library.
 * It is used by transfer-url-copy to set the monitoring information of a transfer
 * and to sent the transfer to the message broker
 */

template <class T>
std::string to_string(T t, std::ios_base & (*f)(std::ios_base&))
{
    std::ostringstream oss;
    oss << f << t;
    return oss.str();
}

struct transfer_completed
{
public:
    transfer_completed() {}
    ~transfer_completed() {}

    std::string agent_fqdn;
    std::string transfer_id;
    std::string endpoint;
    std::string source_srm_version;
    std::string destination_srm_version;
    std::string vo;
    std::string source_url;
    std::string dest_url;
    std::string source_hostname;
    std::string dest_hostname;
    std::string source_site_name;
    std::string dest_site_name;
    std::string t_channel;
    std::string timestamp_transfer_started; //epoch-seconds
    std::string timestamp_transfer_completed; //epoch-seconds
    std::string timestamp_checksum_source_started; //epoch-seconds
    std::string timestamp_checksum_source_ended; //epoch-seconds
    std::string timestamp_checksum_dest_started; //epoch-seconds
    std::string timestamp_checksum_dest_ended; //epoch-seconds
    std::string transfer_timeout;
    std::string checksum_timeout;
    std::string transfer_error_code;
    std::string transfer_error_scope; //source/destination
    std::string transfer_error_message; //text error message
    std::string failure_phase; // (preparation, transfer, checksum, etc)
    std::string transfer_error_category; //persmission, etc
    std::string final_transfer_state; //OK/Error/Aborted
    std::string total_bytes_transfered; // (this will include the info retrieved from the performance markers)
    std::string number_of_streams;
    std::string tcp_buffer_size;
    std::string block_size;
    std::string file_size;
    std::string time_spent_in_srm_preparation_start; //epoc
    std::string time_spent_in_srm_preparation_end; //epoc
    std::string time_spent_in_srm_finalization_start; //epoc
    std::string time_spent_in_srm_finalization_end; //epoc
    std::string srm_space_token_source;
    std::string srm_space_token_dest;
    std::string tr_timestamp_start;
    std::string tr_timestamp_complete;
    std::string channel_type;
    std::string user_dn;
    std::string file_metadata;
    std::string job_metadata;
    std::string retry;
    std::string retry_max;
    std::string job_m_replica;
    std::string job_state;
    std::string is_recoverable;
};

class msg_ifce
{
private:

    enum {
        MSG_IFCE_WAITING_START,
        MSG_IFCE_WAITING_FINISH
    } state;

    static bool instanceFlag;
    static msg_ifce *single;
    std::string errorMessage;
    bool read_initial_config();
    msg_ifce(); /*private constructor*/

public:
    /*static vector<std::string>  getData( const char * id);*/
    std::string getTimestamp();
    std::string SendTransferStartMessage(transfer_completed *tr_started);
    std::string SendTransferFinishMessage(transfer_completed *tr_completed, bool force=false);
    //transfer_completed setters
    void set_agent_fqdn(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_id(transfer_completed* tr_completed, const std::string & value);
    void set_source_srm_version(transfer_completed* tr_completed, const std::string & value);
    void set_destination_srm_version(transfer_completed* tr_completed, const std::string & value);
    void set_source_url(transfer_completed* tr_completed, const std::string & value);
    void set_dest_url(transfer_completed* tr_completed, const std::string & value);
    void set_source_hostname(transfer_completed* tr_completed, const std::string & value);
    void set_dest_hostname(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_transfer_started(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_transfer_completed(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_checksum_source_started(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_checksum_source_ended(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_checksum_dest_started(transfer_completed* tr_completed, const std::string & value);
    void set_timestamp_checksum_dest_ended(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_timeout(transfer_completed* tr_completed, unsigned value);
    void set_checksum_timeout(transfer_completed* tr_completed, int value);
    void set_total_bytes_transfered(transfer_completed* tr_completed, off_t value);
    void set_number_of_streams(transfer_completed* tr_completed, unsigned value);
    void set_tcp_buffer_size(transfer_completed* tr_completed, unsigned value);
    void set_block_size(transfer_completed* tr_completed, unsigned value);
    void set_file_size(transfer_completed* tr_completed, off_t value);
    void set_time_spent_in_srm_preparation_start(transfer_completed* tr_completed, const std::string & value);
    void set_time_spent_in_srm_preparation_end(transfer_completed* tr_completed, const std::string & value);
    void set_time_spent_in_srm_finalization_start(transfer_completed* tr_completed, const std::string & value);
    void set_time_spent_in_srm_finalization_end(transfer_completed* tr_completed, const std::string & value);
    void set_srm_space_token_source(transfer_completed* tr_completed, const std::string & value);
    void set_srm_space_token_dest(transfer_completed* tr_completed, const std::string & value);
    void set_t_channel(transfer_completed* tr_completed, const std::string & value);
    void set_source_site_name(transfer_completed* tr_completed, const std::string & value);
    void set_dest_site_name(transfer_completed* tr_completed, const std::string & value);
    void set_vo(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_error_code(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_error_scope(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_error_message(transfer_completed* tr_completed, const std::string & value);
    void set_failure_phase(transfer_completed* tr_completed, const std::string & value);
    void set_transfer_error_category(transfer_completed* tr_completed, const std::string & value);
    void set_final_transfer_state(transfer_completed* tr_completed, const std::string & value);
    void set_endpoint(transfer_completed* tr_completed, const std::string & value);
    void set_tr_timestamp_start(transfer_completed* tr_completed, const std::string & value);
    void set_tr_timestamp_complete(transfer_completed* tr_completed, const std::string & value);
    void set_channel_type(transfer_completed* tr_completed, const std::string & value);
    void set_user_dn(transfer_completed* tr_completed, const std::string & value);

    void set_file_metadata(transfer_completed* tr_completed, const std::string & value);
    void set_job_metadata(transfer_completed* tr_completed, const std::string & value);

    void set_retry(transfer_completed* tr_completed, const std::string & value);
    void set_retry_max(transfer_completed* tr_completed, const std::string & value);

    void set_job_state(transfer_completed* tr_completed, const std::string & value);
    void set_job_m_replica(transfer_completed* tr_completed, const std::string & value);

    void set_is_recoverable(transfer_completed* tr_completed, bool recoverable);

    static msg_ifce* getInstance();
    ~msg_ifce();
};

#endif
