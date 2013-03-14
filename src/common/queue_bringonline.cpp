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

#include <iostream>
#include <time.h>
#include <ctime>
#include "queue_bringonline.h"


ThreadSafeBringOnlineList::ThreadSafeBringOnlineList() {    
}

ThreadSafeBringOnlineList::~ThreadSafeBringOnlineList() {
}

void ThreadSafeBringOnlineList::push_back(struct message_bringonline msg) {
    ThreadTraits::LOCK lock(_mutex);
    m_list.push_back(msg);
}

void ThreadSafeBringOnlineList::clear() {
    ThreadTraits::LOCK lock(_mutex);
    m_list.clear();
}

void ThreadSafeBringOnlineList::checkExpiredMsg(std::vector<struct message_bringonline>&) {
    ThreadTraits::LOCK lock(_mutex);  
}

void ThreadSafeBringOnlineList::updateMsg(struct message_bringonline) {
    ThreadTraits::LOCK lock(_mutex);    
}

void ThreadSafeBringOnlineList::deleteMsg(std::vector<struct message_bringonline>& messages) {
    ThreadTraits::LOCK lock(_mutex);
    std::list<struct message_bringonline>::iterator i = m_list.begin();
    for (std::vector<struct message_bringonline>::iterator iter = messages.begin(); iter != messages.end(); ++iter) {
        i = m_list.begin();
        while (i != m_list.end()) {
            if ( (*iter).file_id == i->file_id && std::string( (*iter).job_id).compare(std::string(i->job_id)) == 0) {
                m_list.erase(i++);
            } else {
                ++i;
            }
        }
    }
}

void ThreadSafeBringOnlineList::removeFinishedTr(struct message_bringonline msg) {
    ThreadTraits::LOCK lock(_mutex);
    std::list<struct message_bringonline>::iterator i = m_list.begin();
    while (i != m_list.end()) {
        if (msg.file_id == i->file_id && msg.job_id.compare(std::string(i->job_id)) == 0) {
            m_list.erase(i++);
        } else {
            ++i;
        }
    }    
}

