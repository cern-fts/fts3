/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <time.h>
#include <ctime>

#include "definitions.h"
#include "ThreadSafeList.h"


ThreadSafeList::ThreadSafeList()
{
}


ThreadSafeList::~ThreadSafeList()
{
}


std::list<struct MessageUpdater> ThreadSafeList::getList()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<struct MessageUpdater> tempList = m_list;
    return tempList;
}


void ThreadSafeList::push_back(MessageUpdater &msg)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    m_list.push_back(msg);
}


void ThreadSafeList::clear()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    m_list.clear();
}


void ThreadSafeList::checkExpiredMsg(std::vector<struct MessageUpdater> &messages)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<struct MessageUpdater>::iterator iter;
    boost::posix_time::time_duration::tick_type timestamp1;
    boost::posix_time::time_duration::tick_type timestamp2;
    boost::posix_time::time_duration::tick_type n_seconds = 0;
    for (iter = m_list.begin(); iter != m_list.end(); ++iter) {
        timestamp1 = milliseconds_since_epoch();
        timestamp2 = iter->timestamp;
        n_seconds = timestamp1 - timestamp2;
        if (n_seconds > 300000) //5min
        {
            messages.push_back(*iter);
        }
    }
}


void ThreadSafeList::updateMsg(MessageUpdater &msg)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<struct MessageUpdater>::iterator iter;
    for (iter = m_list.begin(); iter != m_list.end(); ++iter) {
        if (msg.file_id == iter->file_id &&
            std::string(msg.job_id).compare(std::string(iter->job_id)) == 0 &&
            msg.process_id == iter->process_id) {
            iter->timestamp = msg.timestamp;
            break;
        }
    }
}


void ThreadSafeList::deleteMsg(std::vector<struct MessageUpdater> &messages)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<struct MessageUpdater>::iterator i = m_list.begin();
    for (std::vector<struct MessageUpdater>::iterator iter = messages.begin(); iter != messages.end(); ++iter) {
        i = m_list.begin();
        while (i != m_list.end()) {
            if ((*iter).file_id == i->file_id && std::string((*iter).job_id).compare(std::string(i->job_id)) == 0) {
                m_list.erase(i++);
            }
            else {
                ++i;
            }
        }
    }
}


void ThreadSafeList::removeFinishedTr(std::string job_id, int file_id)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<struct MessageUpdater>::iterator i = m_list.begin();
    while (i != m_list.end()) {
        if (file_id == i->file_id && job_id == std::string(i->job_id)) {
            m_list.erase(i++);
        }
        else {
            ++i;
        }
    }
}
