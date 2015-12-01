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

#pragma once
#ifndef MESSAGES_H
#define MESSAGES_H

#include <glib.h>
#include <cstring>
#include <boost/date_time/posix_time/posix_time_types.hpp>


#define JOB_ID_LEN 36+1
#define FILE_ID_LEN 36
#define TRANFER_STATUS_LEN 50
#define TRANSFER_MESSAGE 1024
#define MAX_NUM_MSGS 50000
#define SOURCE_SE_ 100
#define DEST_SE_ 100


struct MessageBase
{
public:
    MessageBase(): msg_errno(false)
    {
    }

    int  msg_errno;

    void set_error(int errcode)
    {
        msg_errno = errcode;
    }
};


struct Message: public MessageBase
{
public:

    Message():file_id(0),
        process_id(0),
        timeInSecs(0.0),
        filesize(0),
        nostreams(2),
        timeout(3600),
        buffersize(0),
        timestamp(0),
        retry(false),
        throughput(0.0)
    {
        memset(job_id, 0, sizeof (job_id));
        memset(transfer_status, 0, sizeof (transfer_status));
        memset(transfer_message, 0, sizeof (transfer_message));
        memset(source_se, 0, sizeof (source_se));
        memset(dest_se, 0, sizeof (dest_se));
    }

    ~Message()
    {
    }

    char job_id[JOB_ID_LEN];
    char transfer_status[TRANFER_STATUS_LEN];
    char transfer_message[TRANSFER_MESSAGE];
    char source_se[SOURCE_SE_];
    char dest_se[DEST_SE_];
    int file_id;
    pid_t process_id;
    double timeInSecs;
    double filesize;
    unsigned int nostreams;
    unsigned int timeout;
    unsigned int buffersize;
    boost::posix_time::time_duration::tick_type timestamp;
    bool retry;
    double throughput;
};


struct MessageUpdater: public MessageBase
{
public:
    MessageUpdater():file_id(0),process_id(0),timestamp(0), throughput(0.0), transferred(0.0)
    {
        memset(job_id, 0, sizeof (job_id));
        memset(source_surl, 0, sizeof (source_surl));
        memset(dest_surl, 0, sizeof (dest_surl));
        memset(source_turl, 0, sizeof (source_turl));
        memset(dest_turl, 0, sizeof (dest_turl));
        memset(transfer_status, 0, sizeof (transfer_status));
    }

    ~MessageUpdater()
    {
    }

    char job_id[JOB_ID_LEN];
    int file_id;
    pid_t process_id;
    boost::posix_time::time_duration::tick_type timestamp;
    double throughput;
    double transferred;
    char source_surl[150];
    char dest_surl[150];
    char source_turl[150];
    char dest_turl[150];
    char transfer_status[TRANFER_STATUS_LEN];
};


struct MessageLog: public MessageBase
{
public:
    MessageLog():file_id(0), debugFile(false),timestamp(0)
    {
        memset(job_id, 0, sizeof (job_id));
        memset(host, 0, 255);
        memset(filePath, 0, 1024);
    }

    ~MessageLog()
    {
    }

    char job_id[JOB_ID_LEN];
    int file_id;
    char host[255];
    char filePath[1024];
    bool debugFile;
    boost::posix_time::time_duration::tick_type timestamp;
};


struct MessageBringonline: public MessageBase
{
public:
    MessageBringonline(): file_id(0)
    {
        memset(job_id, 0, sizeof (job_id));
        memset(transfer_status, 0, sizeof (transfer_status));
        memset(transfer_message, 0, sizeof (transfer_message));
    }

    ~MessageBringonline()
    {
    }

    int file_id;
    char job_id[JOB_ID_LEN];
    char transfer_status[TRANFER_STATUS_LEN];
    char transfer_message[TRANSFER_MESSAGE];

};


struct MessageState: public MessageBase
{
public:

    MessageState() : file_id(0), retry_counter(0), retry_max(0)
    {
    }

    ~MessageState()
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
    std::string job_metadata;
    std::string file_metadata;
    std::string timestamp;
    std::string user_dn;
    std::string source_url;
    std::string dest_url;

};


struct MessageMonitoring: public MessageBase
{
public:
    MessageMonitoring():timestamp(0)
    {
        memset(msg, 0, sizeof (msg));
    }

    ~MessageMonitoring()
    {
    }

    char msg[5000];
    boost::posix_time::time_duration::tick_type timestamp;
};


struct MessageSanity
{
public:
    MessageSanity(): revertToSubmitted(false),
        cancelWaitingFiles(false),
        revertNotUsedFiles(false),
        forceFailTransfers(false),
        setToFailOldQueuedJobs(false),
        checkSanityState(false),
        cleanUpRecords(false),
        msgCron(false)
    {
    }

    ~MessageSanity()
    {
    }

    bool revertToSubmitted;
    bool cancelWaitingFiles;
    bool revertNotUsedFiles;
    bool forceFailTransfers;
    bool setToFailOldQueuedJobs;
    bool checkSanityState;
    bool cleanUpRecords;
    bool msgCron;
};


#endif // MESSAGES_H
