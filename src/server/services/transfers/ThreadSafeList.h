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

#ifndef THREADSAFELIST_H_
#define THREADSAFELIST_H_

#include <list>
#include <vector>
#include <string>
#include <boost/thread.hpp>
#include "msg-bus/events.h"

class ThreadSafeList
{
public:

    static ThreadSafeList& get_instance()
    {
        static ThreadSafeList instance;
        return instance;
    }
    ThreadSafeList();
    ~ThreadSafeList();

    void push_back(fts3::events::MessageUpdater &msg);
    void clear();
    void updateMsg(fts3::events::MessageUpdater &msg);
    void checkExpiredMsg(std::vector<fts3::events::MessageUpdater>& messages,
        boost::posix_time::time_duration timeout);
    void deleteMsg(std::vector<fts3::events::MessageUpdater>& messages);
    void removeFinishedTr(std::string job_id, uint64_t file_id);

private:
    std::list<fts3::events::MessageUpdater> m_list;
    mutable boost::recursive_timed_mutex _mutex;
};

#endif /*THREADSAFELIST_H_*/
