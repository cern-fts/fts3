#include "mq_manager.h"


    // ctor for consumer (fts3_server)
    QueueManager::QueueManager() : drop_(false)
    {
      remove();
      mq_.reset(new message_queue(create_only, FTS3_MQ_NAME, MAX_NUM_MSGS, sizeof(message)));
    }
 
    // ctor for producer (fts3_url_copy)
    QueueManager::QueueManager(std::string job_id, std::string file_id) : drop_(true), mq_(new message_queue(open_only, FTS3_MQ_NAME)) {
    	job_id = std::string("");
	file_id = std::string("");
    }
 
    QueueManager::~QueueManager() { if(drop_) remove(); }
 
    void QueueManager::send(struct message* msg)
    {
      mq_->send(msg, sizeof(message), 0);
    }
 
    struct message* QueueManager::receive()
    {
      struct message* msg = new struct message;
 
      unsigned int priority;
      std::size_t recvd_size;
      mq_->receive(msg, sizeof(message), recvd_size, priority);
 
      return msg;
    }
 
 

void QueueManager::remove() { message_queue::remove(FTS3_MQ_NAME); }
