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

#include "mq_manager.h"
#include "common/logger.h"
#include "common/error.h"
#include <boost/scoped_ptr.hpp>

using namespace boost::interprocess;
boost::scoped_ptr<message_queue> mq_;
boost::scoped_ptr<message_queue> mq_mon;
boost::scoped_ptr<message_queue> mq_updater;

using namespace FTS3_COMMON_NAMESPACE;


QueueManager::QueueManager(bool consumer) : drop_(false), init(false) {
    if (consumer == true) {
        mq_.reset(new message_queue(create_only, FTS3_MQ_NAME, MAX_NUM_MSGS, sizeof (message)));
        std::string path = "/dev/shm/fts3mq";
        chmod(path.c_str(), 0777);
    } else { //fts_url_copy
        mq_.reset(new message_queue(open_only, FTS3_MQ_NAME));
    }
}

QueueManager::QueueManager(bool, bool consumer) : drop_(false), init(false) {
    const std::string path = "/dev/shm/fts3mqupdater";
    try {
        if (consumer == true) {
            mq_updater.reset(new message_queue(create_only, FTS3_MQ_NAME_UPDATER, MAX_NUM_MSGS, sizeof (message_updater)));
            chmod(path.c_str(), 0777);
        } else { //fts_url_copy
            mq_updater.reset(new message_queue(open_only, FTS3_MQ_NAME_UPDATER));
        }
    } catch (...) {
        mq_updater.reset(new message_queue(open_only, FTS3_MQ_NAME_UPDATER));
    }
}

QueueManager::QueueManager(bool consumer, std::string, bool) : drop_(false), init(false) {
    const std::string path = "/dev/shm/fts3mqmon";
    try {
        if (consumer == true) {
            mq_mon.reset(new message_queue(create_only, FTS3_MQ_NAME_MON, MAX_NUM_MSGS, 3000));
            chmod(path.c_str(), 0777);
        } else { //fts_url_copy
            mq_mon.reset(new message_queue(open_only, FTS3_MQ_NAME_MON));
        }
    } catch (...) {
        mq_mon.reset(new message_queue(open_only, FTS3_MQ_NAME_MON));
    }
}

QueueManager::~QueueManager() {
}

void QueueManager::sendUpdater(struct message_updater* msg) {
    int counter = 0;
    bool sent = false;
    while (counter < 550 && sent == false) {
        try {
            sent = mq_updater->try_send(msg, sizeof (message_updater), 0);
            usleep(1000);
            counter++;
        } catch (interprocess_exception &ex) {
            usleep(1000);
            counter++;
        } catch (...) {
            usleep(1000);
            counter++;
        }
    }
}

bool QueueManager::receiveUpdater(struct message_updater* msg) {
    unsigned int priority;
    std::size_t recvd_size;
    bool receive = false;

        try {
            receive = mq_updater->try_receive(msg, sizeof (message_updater), recvd_size, priority);
	    usleep(20000);
        } catch (interprocess_exception &ex) {
	    usleep(20000);
            receive = false;
        } catch (...) {
	    usleep(20000);
            receive = false;
        }
 return receive;	  
}

void QueueManager::send(struct message* msg) {
    int counter = 0;
    bool sent = false;
    while (counter < 550 && sent == false) {
        try {
            sent = mq_->try_send(msg, sizeof (message), 0);
            usleep(1000);
            counter++;
        } catch (interprocess_exception &ex) {
            usleep(1000);
            counter++;
        } catch (...) {
            usleep(1000);
            counter++;
        }
    }
}



bool QueueManager::receive(struct message* msg) {
    unsigned int priority;
    std::size_t recvd_size;
    bool receive = false;

        try {
            receive = mq_->try_receive(msg, sizeof (message), recvd_size, priority);
	    usleep(20000);
        } catch (interprocess_exception &ex) {
	    usleep(20000);
            receive = false;
        } catch (...) {
	    usleep(20000);
            receive = false;
        }
	
 return receive;	
}

void QueueManager::msg_send(const char* msg) {
    int counter = 0;
    bool sent = false;
    char json[3000] = {0};
    strcpy(json, msg);

    while (counter < 550 && sent == false) {
        try {
            sent = mq_mon->try_send(json, 3000, 0);
            usleep(1000);
            counter++;
        } catch (interprocess_exception &ex) {
            usleep(1000);
            counter++;
        } catch (...) {
            usleep(1000);
            counter++;
        }
    }
}



bool QueueManager::msg_receive(char* msg) {
    unsigned int priority;
    std::size_t recvd_size;
    bool receive = false;

        try {
            char json[3000] = {0};
            receive = mq_mon->try_receive(json, 3000, recvd_size, priority);
            if (receive) {
                strcpy(msg, json);
            }
	    usleep(1000);
        } catch (interprocess_exception &ex) {
            usleep(1000);
            receive = false;
        } catch (...) {
            usleep(1000);
            receive = false;
        }
    return receive;
}


void QueueManager::remove() {
    /*message_queue::remove(FTS3_MQ_NAME);*/
}
