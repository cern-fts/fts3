/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctime>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#define JOB_ID_LEN 36+1
#define FILE_ID_LEN 36
#define TRANFER_STATUS_LEN 50
#define TRANSFER_MESSAGE 1024
#define MAX_NUM_MSGS 50000
#define SOURCE_SE_ 100
#define DEST_SE_ 100

#define FTS3_MQ_NAME "fts3mq"
#define FTS3_MQ_NAME_UPDATER "fts3mqupdater"
#define FTS3_MQ_NAME_MON "fts3mqmon"

using namespace std;

struct message {
public:

    message() {
        memset(job_id, 0, sizeof (job_id));
        file_id = 0;
        memset(transfer_status, 0, sizeof (transfer_status));
        memset(transfer_message, 0, sizeof (transfer_message));
        process_id = 0;
        timeInSecs = 0.0;
        filesize = 0.0;
        nostreams = 2;
        timeout = 3600;
        buffersize = 0;
        memset(source_se, 0, sizeof (source_se));
        memset(dest_se, 0, sizeof (dest_se));	
    }

    ~message() {
    }
    char job_id[JOB_ID_LEN];
    int file_id;
    char transfer_status[TRANFER_STATUS_LEN];
    char transfer_message[TRANSFER_MESSAGE];
    pid_t process_id;
    double timeInSecs;
    double filesize;
    unsigned int nostreams;
    unsigned int timeout;
    unsigned int buffersize;
    char source_se[SOURCE_SE_];
    char dest_se[DEST_SE_];
    boost::posix_time::time_duration::tick_type timestamp;



};

struct message_updater {
public:
    message_updater() {
        memset(job_id, 0, sizeof (job_id));
        file_id = 0;
        process_id = 0;       
    }

    ~message_updater() {
    }
    char job_id[JOB_ID_LEN];
    int file_id;
    pid_t process_id;
    boost::posix_time::time_duration::tick_type timestamp;
};


struct message_bringonline {
public:
    message_bringonline() {
    	job_id = std::string();
    	file_id = 0;
    	url = std::string();
        timestamp = time(NULL);
        retries = 0;       
        proxy = std::string();
	started = false;
	token = std::string();
    }

    ~message_bringonline() {
    }
    std::string job_id;
    int file_id;
    std::string url;
    time_t timestamp; 
    int retries;
    std::string proxy;
    bool started;
    std::string token;
};



#define DEFAULT_TIMEOUT 3600
#define MID_TIMEOUT 5000
const int timeouts[] = {4000, 5000, 6000, 9000, 11000, 13000, 14000};
const size_t timeoutslen = (sizeof (timeouts) / sizeof *(timeouts));

#define DEFAULT_NOSTREAMS 4
const int nostreams[] = {1, 2, 4, 6, 8, 10, 12};
const size_t nostreamslen = (sizeof (nostreams) / sizeof *(nostreams));


#define DEFAULT_BUFFSIZE 0
const int buffsizes[] = {1048576, 4194304, 5242880, 7340032, 8388608, 9437184, 11534336, 12582912, 14680064, 67108864};
const size_t buffsizeslen = (sizeof (buffsizes) / sizeof *(buffsizes));





