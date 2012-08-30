#pragma once


#define JOB_ID_LEN 36+1
#define FILE_ID_LEN 36
#define TRANFER_STATUS_LEN 50
#define TRANSFER_MESSAGE 1024
#define MAX_NUM_MSGS 50000
#define SOURCE_SE_ 100
#define DEST_SE_ 100

#define FTS3_MQ_NAME "fts3mq"

struct message{  
      char job_id[JOB_ID_LEN];
      char file_id[FILE_ID_LEN];
      char transfer_status[TRANFER_STATUS_LEN];
      char transfer_message[TRANSFER_MESSAGE];
      unsigned int process_id;
      double timeInSecs;
      double filesize;
      unsigned int nostreams;
      unsigned int timeout;
      unsigned int buffersize;
      char source_se[SOURCE_SE_];
      char dest_se[DEST_SE_];      
};
 
 
#define DEFAULT_TIMEOUT 3600
const int timeouts[] = {4000, 5000, 6000,  9000, 11000,  13000, 14000};
const size_t timeoutslen = (sizeof (timeouts) / sizeof *(timeouts));

#define DEFAULT_NOSTREAMS 8
const int nostreams[] = {4, 6, 8, 10, 12};
const size_t nostreamslen = (sizeof (nostreams) / sizeof *(nostreams));


#define DEFAULT_BUFFSIZE 0
const int buffsizes[] = {4194304, 5242880,  7340032, 8388608, 9437184,  11534336, 12582912,  14680064	};
const size_t buffsizeslen = (sizeof (buffsizes) / sizeof *(buffsizes));





