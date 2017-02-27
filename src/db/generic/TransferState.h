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

#ifndef DB_TRANSFER_STATE_H
#define DB_TRANSFER_STATE_H

#include <string>

struct TransferState
{
    TransferState() : file_id(0), retry_counter(0), retry_max(0), user_filesize(0), timestamp(0),
        submit_time(0), staging_start(0), staging_finished(0), staging(false)
    {
    }

    std::string vo_name;
    std::string source_se;
    std::string dest_se;
    std::string job_id;
    int file_id;
    std::string job_state;
    std::string file_state;
    int retry_counter;
    int retry_max;
    int64_t user_filesize;
    std::string job_metadata;
    std::string file_metadata;
    uint64_t    timestamp;
    uint64_t    submit_time;
    uint64_t    staging_start;
    uint64_t    staging_finished;
    bool staging;
    std::string user_dn;
    std::string source_url;
    std::string dest_url;
    std::string reason;

};

#endif // DB_TRANSFER_STATE_H
