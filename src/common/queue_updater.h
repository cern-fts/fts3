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

#ifndef THREADSAFELIST_H_
#define THREADSAFELIST_H_

#include <list>
#include <vector>
#include <string>
#include "definitions.h"
#include "producer_consumer_common.h"

using namespace FTS3_COMMON_NAMESPACE;

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

    std::list<struct message_updater> getList();
    void push_back(message_updater &msg);
    void clear();
    void updateMsg(message_updater &msg);
    void checkExpiredMsg(std::vector<struct message_updater>& messages);
    void deleteMsg(std::vector<struct message_updater>& messages);
    void removeFinishedTr(std::string job_id, int file_id);
    bool isAlive(int fileID);

private:
    std::list<struct message_updater> m_list;
    mutable boost::recursive_mutex _mutex;
};

#endif /*THREADSAFELIST_H_*/
