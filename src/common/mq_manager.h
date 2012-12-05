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

#include <iostream>
#include <cstring>
#include <vector>
#include <string>
#include "definitions.h"
#include <boost/interprocess/ipc/message_queue.hpp> 

 
class QueueManager
  {
  private:
    bool drop_; // drop the message queue
    bool init;
 
  public:  
    void remove();

    QueueManager(bool consumer);  
    QueueManager(bool consumer, std::string qname, bool);
    QueueManager(bool active, bool consumer);      
 
    ~QueueManager();
 
    void send(struct message* msg);

    void t_send(struct message* msg);
 
    void receive(struct message* msg) throw(boost::interprocess::interprocess_exception);
    
    void msg_send(const char* msg);

    void msg_t_send(const char* msg);
 
    void msg_receive(char* msg) throw(boost::interprocess::interprocess_exception);
    
    void sendUpdater(struct message_updater* msg); 
    void receiveUpdater(struct message_updater* msg) throw(boost::interprocess::interprocess_exception);   

  };

