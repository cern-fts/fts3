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
#include "UtilityRoutines.h"
#include "msg-bus/producer.h"
#include "db/generic/TransferState.h"

/**
 * This is the external interface of the FTS messaging library.
 * It is used by transfer-url-copy to set the monitoring information of a transfer
 * and to sent the transfer to the message broker
 */

struct TransferCompleted
{
public:
    TransferCompleted():
        transfer_timeout(0), checksum_timeout(0), total_bytes_transfered(0), number_of_streams(0), tcp_buffer_size(0),
        block_size(0), file_size(0), retry(0), retry_max(0), is_recoverable(false)
    {}
    ~TransferCompleted() {}

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
    unsigned    transfer_timeout;
    int         checksum_timeout;
    std::string transfer_error_code;
    std::string transfer_error_scope; //source/destination
    std::string transfer_error_message; //text error message
    std::string failure_phase; // (preparation, transfer, checksum, etc)
    std::string transfer_error_category; //persmission, etc
    std::string final_transfer_state; //OK/Error/Aborted
    off_t       total_bytes_transfered; // (this will include the info retrieved from the performance markers)
    int         number_of_streams;
    unsigned    tcp_buffer_size;
    unsigned    block_size;
    off_t       file_size;
    std::string time_spent_in_srm_preparation_start; //epoch
    std::string time_spent_in_srm_preparation_end; //epoch
    std::string time_spent_in_srm_finalization_start; //epoch
    std::string time_spent_in_srm_finalization_end; //epoch
    std::string srm_space_token_source;
    std::string srm_space_token_dest;
    std::string tr_timestamp_start;
    std::string tr_timestamp_complete;
    std::string channel_type;
    std::string user_dn;
    std::string file_metadata;
    std::string job_metadata;
    unsigned    retry;
    unsigned    retry_max;
    std::string job_m_replica;
    std::string job_state;
    bool        is_recoverable;
};

class MsgIfce
{
private:

    enum {
        MSG_IFCE_WAITING_START,
        MSG_IFCE_WAITING_FINISH
    } state;

    static bool instanceFlag;
    static MsgIfce *single;
    MsgIfce(); /*private constructor*/

public:
    std::string SendTransferStartMessage(Producer &producer, const TransferCompleted &tr_started);
    std::string SendTransferFinishMessage(Producer &producer, const TransferCompleted &tr_completed, bool force=false);
    std::string SendTransferStatusChange(Producer &producer, const TransferState &tr_state);

    static MsgIfce * getInstance();
    ~MsgIfce();
};

#endif
