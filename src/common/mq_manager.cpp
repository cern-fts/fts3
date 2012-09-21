#include "mq_manager.h"
//#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>
 
using namespace boost::interprocess;
boost::scoped_ptr<message_queue> mq_; 

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
 
    void QueueManager::send(struct message* msg) throw(interprocess_exception)
    {
     try{
      mq_->send(msg, sizeof(message), 0);
     }catch(interprocess_exception &ex){
      throw ex;
     }
    }

    void QueueManager::t_send(struct message* msg) throw(interprocess_exception)
    {
     try{
      boost::posix_time::ptime start(boost::posix_time::second_clock::local_time());
      boost::posix_time::ptime end = start + boost::posix_time::hours(1); 
      mq_->timed_send(msg, sizeof(message), 0, end);
     }catch(interprocess_exception &ex){
      throw ex;
     }
    }


 
    void QueueManager::receive(struct message* msg) throw(interprocess_exception)
    {
      unsigned int priority;
      std::size_t recvd_size; 
      try{
      mq_->receive(msg, sizeof(message), recvd_size, priority);
      }catch(interprocess_exception &ex){
        throw ex;
     }
    }
 
 

void QueueManager::remove() { /*message_queue::remove(FTS3_MQ_NAME);*/ }
