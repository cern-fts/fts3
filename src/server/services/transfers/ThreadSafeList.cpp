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
#include <ctime>

#include "common/definitions.h"
#include "common/Logger.h"
#include "common/PidTools.h"
#include "ThreadSafeList.h"


ThreadSafeList::ThreadSafeList()
{
}


ThreadSafeList::~ThreadSafeList()
{
}


std::list<fts3::events::MessageUpdater> ThreadSafeList::getList()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<fts3::events::MessageUpdater> tempList = m_list;
    return tempList;
}


void ThreadSafeList::push_back(fts3::events::MessageUpdater &msg)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    m_list.push_back(msg);
}


void ThreadSafeList::clear()
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    m_list.clear();
}


void ThreadSafeList::checkExpiredMsg(std::vector<fts3::events::MessageUpdater> &messages)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    boost::posix_time::time_duration::tick_type timestamp1;
    boost::posix_time::time_duration::tick_type timestamp2;
    boost::posix_time::time_duration::tick_type n_milliseconds = 0;
    for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
        timestamp1 = milliseconds_since_epoch();
        timestamp2 = iter->timestamp();
        n_milliseconds = timestamp1 - timestamp2;
        if (n_milliseconds > 300000) //5min
        {
            messages.push_back(*iter);
        }
    }
}


void ThreadSafeList::updateMsg(fts3::events::MessageUpdater &msg)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<fts3::events::MessageUpdater>::iterator iter;
    uint64_t pidStartTime = fts3::common::getPidStartime(msg.process_id());
    pidStartTime *= 1000; // timestamp() is in milliseconds
    for (iter = m_list.begin(); iter != m_list.end(); ++iter) {

        if (msg.process_id() == iter->process_id()) {
            if (pidStartTime > 0 && msg.timestamp() >= pidStartTime) {
                iter->set_timestamp(msg.timestamp());
            }
            else if (pidStartTime > 0){
                FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                    << "Found a matching pid, but start time is more recent than last known message"
                    << "(" << pidStartTime << " vs " << msg.timestamp() << " for " << msg.process_id() << ")"
                    << fts3::common::commit;
            }
        }
    }
}


void ThreadSafeList::deleteMsg(std::vector<fts3::events::MessageUpdater> &messages)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<fts3::events::MessageUpdater>::iterator i = m_list.begin();
    for (auto iter = messages.begin(); iter != messages.end(); ++iter) {
        i = m_list.begin();
        while (i != m_list.end()) {
            if ((*iter).file_id() == i->file_id() && (*iter).job_id().compare(i->job_id()) == 0) {
                m_list.erase(i++);
            }
            else {
                ++i;
            }
        }
    }
}


void ThreadSafeList::removeFinishedTr(std::string job_id, uint64_t file_id)
{
    boost::recursive_mutex::scoped_lock lock(_mutex);
    std::list<fts3::events::MessageUpdater>::iterator i = m_list.begin();
    while (i != m_list.end()) {
        if (file_id == i->file_id() && job_id == i->job_id()) {
            m_list.erase(i++);
        }
        else {
            ++i;
        }
    }
}
