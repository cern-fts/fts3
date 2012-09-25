#include "mq_manager.h"
//#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/scoped_ptr.hpp>

using namespace boost::interprocess;
boost::scoped_ptr<message_queue> mq_;
boost::scoped_ptr<message_queue> mq_mon;

struct msg_message {
    char json[5120];
};

QueueManager::QueueManager(bool consumer) : drop_(false), init(false) {
    //fts3_server
    if (consumer == true) {
        //remove();
        mq_.reset(new message_queue(create_only, FTS3_MQ_NAME, MAX_NUM_MSGS, sizeof (message)));
        std::string path = "/dev/shm/fts3mq";
        chmod(path.c_str(), 0777);
    } else { //ft3_url_copy
        mq_.reset(new message_queue(open_only, FTS3_MQ_NAME));
    }
}

QueueManager::QueueManager(bool consumer, std::string qname) : drop_(false), init(false) {
    const std::string path = "/dev/shm/fts3mqmon";
    try {
        //fts3_server
        if (consumer == true) {
            //remove();
            mq_mon.reset(new message_queue(create_only, "fts3mqmon", MAX_NUM_MSGS, sizeof (msg_message)));
            chmod(path.c_str(), 0777);
        } else { //ft3_url_copy
            mq_mon.reset(new message_queue(open_only, "fts3mqmon"));
        }
    } catch (...) {
        mq_mon.reset(new message_queue(open_only, "fts3mqmon"));
    }
}

QueueManager::~QueueManager() {
}

void QueueManager::send(struct message* msg) throw (interprocess_exception) {
    try {
        mq_->send(msg, sizeof (message), 0);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::t_send(struct message* msg) throw (interprocess_exception) {
    try {
        boost::posix_time::ptime start(boost::posix_time::second_clock::local_time());
        boost::posix_time::ptime end = start + boost::posix_time::hours(1);
        mq_->timed_send(msg, sizeof (message), 0, end);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::receive(struct message* msg) throw (interprocess_exception) {
    unsigned int priority;
    std::size_t recvd_size;
    try {
        mq_->receive(msg, sizeof (message), recvd_size, priority);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::msg_send(const char* msg) throw (interprocess_exception) {
    try {
        struct msg_message m;
        strcpy(m.json, msg);

        mq_mon->send(&m, sizeof (msg_message), 0);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::msg_t_send(const char* msg) throw (interprocess_exception) {
    try {
        struct msg_message m;
        strcpy(m.json, msg);
        boost::posix_time::ptime start(boost::posix_time::second_clock::local_time());
        boost::posix_time::ptime end = start + boost::posix_time::minutes(1);
        mq_mon->timed_send(&m, sizeof (msg_message), 0, end);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::msg_receive(char* msg) throw (interprocess_exception) {
    unsigned int priority;
    std::size_t recvd_size;
    try {
        struct msg_message m;
        mq_mon->receive(&m, sizeof (msg_message), recvd_size, priority);
        strcpy(msg, m.json);
    } catch (interprocess_exception &ex) {
        throw ex;
    }
}

void QueueManager::remove() {
    /*message_queue::remove(FTS3_MQ_NAME);*/
}
