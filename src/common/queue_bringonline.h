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

#ifndef BRINGONLINEQUEUE_H_
#define BRINGONLINEQUEUE_H_

#include <list>
#include <vector>
#include <string>
#include "definitions.h"
#include "threadtraits.h"

using namespace FTS3_COMMON_NAMESPACE;

class ThreadSafeBringOnlineList
{
public:

    static ThreadSafeBringOnlineList& get_instance()
    {
        static ThreadSafeBringOnlineList instance;
        return instance;
    }
    ThreadSafeBringOnlineList();
    ~ThreadSafeBringOnlineList();

    void push_back(struct message_bringonline message);
    void clear();
    void updateMsg(struct message_bringonline msg);
    void checkExpiredMsg(std::vector<struct message_bringonline>& messages);
    void deleteMsg(std::vector<struct message_bringonline>& messages);
    void removeFinishedTr(struct message_bringonline message);
    std::list<struct message_bringonline> m_list;

private:
    mutable ThreadTraits::MUTEX _mutex;
};

#endif /*BRINGONLINEQUEUE_H_*/
