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
#include <common/Exceptions.h>

#include "common/Logger.h"
#include "common/PidTools.h"
#include "ThreadSafeList.h"

using fts3::common::SystemError;


ThreadSafeList::ThreadSafeList()
{
}


ThreadSafeList::~ThreadSafeList()
{
}


void ThreadSafeList::push_back(fts3::events::MessageUpdater &msg)
{
    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }
    m_list.push_back(msg);
    _mutex.unlock();
}


void ThreadSafeList::clear()
{
    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }
    m_list.clear();
    _mutex.unlock();
}


void ThreadSafeList::checkExpiredMsg(std::vector<fts3::events::MessageUpdater> &messages,
    boost::posix_time::time_duration timeout)
{
    static const boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));

    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }

    try {
        for (auto iter = m_list.begin(); iter != m_list.end(); ++iter) {
            auto nowTime = boost::posix_time::microsec_clock::universal_time();
            auto lastMsgTime = epoch + boost::posix_time::milliseconds(iter->timestamp());

            auto age = nowTime - lastMsgTime;

            if (age > timeout) {
                messages.push_back(*iter);
            }
        }
    }
    catch (...) {
        _mutex.unlock();
        throw;
    }
    _mutex.unlock();
}


void ThreadSafeList::updateMsg(fts3::events::MessageUpdater &msg)
{
    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }

    try {
        std::list<fts3::events::MessageUpdater>::iterator iter;
        uint64_t pidStartTime = fts3::common::getPidStartime(msg.process_id());
        for (iter = m_list.begin(); iter != m_list.end(); ++iter) {

            if (msg.process_id() == iter->process_id()) {
                if (pidStartTime > 0 && msg.timestamp() >= pidStartTime) {
                    iter->set_timestamp(msg.timestamp());
                }
                else if (pidStartTime > 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(WARNING)
                        << "Found a matching pid, but start time is more recent than last known message"
                        << "(" << pidStartTime << " vs " << msg.timestamp() << " for " << msg.process_id() << ")"
                        << fts3::common::commit;
                }
            }
        }
    }
    catch (...) {
        _mutex.unlock();
        throw;
    }
    _mutex.unlock();
}


void ThreadSafeList::deleteMsg(std::vector<fts3::events::MessageUpdater> &messages)
{
    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }

    try {
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
    catch (...) {
        _mutex.unlock();
        throw;
    }
    _mutex.unlock();
}


void ThreadSafeList::removeFinishedTr(std::string job_id, uint64_t file_id)
{
    if (!_mutex.timed_lock(boost::posix_time::seconds(10))) {
        throw SystemError(std::string(__func__) + ": Mutex timeout expired");
    }

    try {
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
    catch (...) {
        _mutex.unlock();
        throw;
    }
    _mutex.unlock();
}
