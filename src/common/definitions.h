#pragma once


#define JOB_ID_LEN 36
#define FILE_ID_LEN 36
#define TRANFER_STATUS_LEN 50
#define TRANSFER_MESSAGE 1024
#define MAX_NUM_MSGS 50000
#define FTS3_MQ_NAME "fts3mq"

struct message{  
      char job_id[JOB_ID_LEN];
      char file_id[FILE_ID_LEN];
      char transfer_status[TRANFER_STATUS_LEN];
      char transfer_message[TRANSFER_MESSAGE];
      int process_id;
};
 
