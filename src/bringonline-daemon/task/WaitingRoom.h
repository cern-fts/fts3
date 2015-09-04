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

#ifndef WAITINGROOM_H_
#define WAITINGROOM_H_

#include "../task/Gfal2Task.h"

#include "common/ThreadPool.h"

#include <boost/ptr_container/ptr_list.hpp> // think about using a lockfree queue
#include <boost/thread.hpp>

using namespace fts3::common;

extern bool stopThreads;

/**
 * A waiting room for task that will be executed in a while
 */
template<typename TASK, typename BASE = Gfal2Task>
class WaitingRoom
{

public:

    /**
     * Singleton instance
     */
    static WaitingRoom& instance()
    {
        static WaitingRoom instance;
        return instance;
    }

    /**
     * Puts new task into the waiting room
     *
     * @param task : a staging task
     */
    void add(TASK* task)
    {
        boost::mutex::scoped_lock lock(m);
        tasks.push_back(task);
    }

    /**
     * Attaches the waiting room to a threadpool
     *
     * @param pool : the thread-pool that will be used to start tasks
     */
    void attach(ThreadPool<BASE>& pool)
    {
        this->pool = &pool;
        // now we can start the thread
        t.reset(new boost::thread(run));
    }

    /**
     * Destructor
     */
    virtual ~WaitingRoom() {}

private:

    /**
     * This routine is executed in a separate thread
     */
    static void run();

    /**
     * Default constructor
     */
    WaitingRoom(): pool(NULL) {}

    /**
     * Copy constructor (not implemented)
     */
    WaitingRoom(WaitingRoom const &);

    /**
     * Assigment operator (not implemented)
     */
    WaitingRoom& operator=(WaitingRoom const &);

    /// the queue with tasks that are waiting
    boost::ptr_list<TASK> tasks;
    /// the mutex preventing concurrent access
    boost::mutex m;
    /// the worker thread
    std::unique_ptr<boost::thread> t;
    /// the threadpool items are waiting for
    ThreadPool<BASE> * pool;
};

template <typename TASK, typename BASE>
void WaitingRoom<TASK, BASE>::run()
{
    WaitingRoom<TASK, BASE>& me = WaitingRoom<TASK, BASE>::instance();

    while (true)
        {
            if(stopThreads) return;//either  gracefully or not

            {
                // lock the mutex
                boost::mutex::scoped_lock lock(me.m);
                // get current time
                time_t now = time(NULL);
                // iterate over all all tasks
                typename boost::ptr_list<TASK>::iterator it, next = me.tasks.begin();
                while ((it = next) != me.tasks.end())
                    {
                        if(stopThreads)
                            return;
                        // next item to check
                        ++next;
                        // if the time has not yet come for the task simply continue
                        if (it->waiting(now)) continue;
                        // otherwise start the task
                        me.pool->start(me.tasks.release(it).release());
                    }
            }
            // wait a while before checking again
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
        }
}

#endif /* WAITINGROOM_H_ */
