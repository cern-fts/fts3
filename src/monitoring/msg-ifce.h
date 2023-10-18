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
        file_id(0), timestamp_transfer_started(0), timestamp_transfer_completed(0), timestamp_checksum_source_started(0),
        timestamp_checksum_source_ended(0), timestamp_checksum_dest_started(0), timestamp_checksum_dest_ended(0),
        transfer_timeout(0), checksum_timeout(0), transfer_error_code(0), final_transfer_state_flag(-1),
        total_bytes_transferred(0),
        number_of_streams(0), tcp_buffer_size(0), block_size(0), scitag(0),
        file_size(0), throughput_bps(0),
        time_spent_in_srm_preparation_start(0), time_spent_in_srm_preparation_end(0),
        time_spent_in_srm_finalization_start(0), time_spent_in_srm_finalization_end(0),
        tr_timestamp_start(0), tr_timestamp_complete(0),
        transfer_time_ms(0), operation_time_ms(0),
        srm_preparation_time_ms(0), srm_finalization_time_ms(0),
        srm_overhead_time_ms(0), srm_overhead_percentage(0),
        checksum_source_time_ms(0), checksum_dest_time_ms(0),
        retry(0), retry_max(0),
        job_m_replica(false), job_multihop(false), is_lasthop(false), is_archiving(false),
        is_recoverable(false), ipv6(false), eviction_code(-1), cleanup_code(-1)
    {}

    ~TransferCompleted() = default;

    std::string transfer_id;
    std::string job_id;
    uint64_t file_id;
    std::string endpoint;
    std::string source_srm_version;
    std::string destination_srm_version;
    std::string vo;
    std::string source_url;
    std::string dest_url;
    std::string source_hostname;
    std::string dest_hostname;
    std::string source_se;
    std::string dest_se;
    std::string protocol;
    std::string source_site_name;
    std::string dest_site_name;
    std::string t_channel;
    uint64_t    timestamp_transfer_started;
    uint64_t    timestamp_transfer_completed;
    uint64_t    timestamp_checksum_source_started;
    uint64_t    timestamp_checksum_source_ended;
    uint64_t    timestamp_checksum_dest_started;
    uint64_t    timestamp_checksum_dest_ended;
    unsigned    transfer_timeout;
    int         checksum_timeout;
    int         transfer_error_code;
    std::string transfer_error_scope; //source/destination
    std::string transfer_error_message; //text error message
    std::string failure_phase; // (preparation, transfer, checksum, etc)
    std::string transfer_error_category; //permission, etc
    std::string final_transfer_state; //OK/Error/Abort
    int         final_transfer_state_flag; // 1/0/-1
    off_t       total_bytes_transferred; // (this will include the info retrieved from the performance markers)
    int         number_of_streams;
    unsigned    tcp_buffer_size;
    unsigned    block_size;
    unsigned    scitag;
    off_t       file_size;
    double      throughput_bps;
    uint64_t    time_spent_in_srm_preparation_start;
    uint64_t    time_spent_in_srm_preparation_end;
    uint64_t    time_spent_in_srm_finalization_start;
    uint64_t    time_spent_in_srm_finalization_end;
    std::string srm_space_token_source;
    std::string srm_space_token_dest;
    uint64_t    tr_timestamp_start;
    uint64_t    tr_timestamp_complete;
    int64_t     transfer_time_ms;
    int64_t     operation_time_ms;
    int64_t     srm_preparation_time_ms;
    int64_t     srm_finalization_time_ms;
    uint64_t    srm_overhead_time_ms;
    double      srm_overhead_percentage;
    int64_t     checksum_source_time_ms;
    int64_t     checksum_dest_time_ms;
    std::string channel_type;
    std::string user_dn;
    std::string file_metadata;
    std::string job_metadata;
    unsigned    retry;
    unsigned    retry_max;
    bool        job_m_replica;
    bool        job_multihop;
    bool        is_lasthop;
    bool        is_archiving;
    std::string job_state;
    bool        is_recoverable;
    bool        ipv6;
    int         eviction_code;
    int         cleanup_code;
    std::string ipver;
    std::string final_destination;
    std::string transfer_type;
    std::string auth_method;
};


struct OptimizerInfo {
public:
    std::string source_se;
    std::string dest_se;

    time_t timestamp;
    double throughput;
    time_t avgDuration;
    double successRate;
    int retryCount;
    int activeCount;
    int queueSize;
    double ema;
    double filesizeAvg, filesizeStdDev;
    int connections;

    std::string rationale;
};

class MsgIfce
{
private:
    static bool instanceFlag;
    static MsgIfce *single;
    MsgIfce(); /*private constructor*/

public:
    std::string SendTransferStartMessage(Producer &producer, const TransferCompleted &tr_started);
    std::string SendTransferFinishMessage(Producer &producer, const TransferCompleted &tr_completed);
    std::string SendTransferStatusChange(Producer &producer, const TransferState &tr_state);
    std::string SendOptimizer(Producer &producer, const OptimizerInfo &opt_info);

    static MsgIfce * getInstance();
    ~MsgIfce();
};

#endif
