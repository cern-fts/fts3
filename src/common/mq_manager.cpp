#include "mq_manager.h"


    QueueManager::QueueManager(bool consumer) : drop_(false)
    {
    	//fts3_server
      if(consumer == true){
	      remove();
	      mq_.reset(new message_queue(create_only, FTS3_MQ_NAME, MAX_NUM_MSGS, sizeof(message)));
      }
      else{ //ft3_url_copy
	       mq_.reset(new message_queue(open_only, FTS3_MQ_NAME));      
      }
    }
 
   
    QueueManager::~QueueManager() {}
 
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
