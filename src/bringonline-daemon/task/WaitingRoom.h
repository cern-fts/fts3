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

#pragma once
#ifndef WAITINGROOM_H_
#define WAITINGROOM_H_

#include <boost/ptr_container/ptr_list.hpp> // think about using a lockfree queue
#include <boost/thread.hpp>

#include "common/ThreadPool.h"

#include "../task/Gfal2Task.h"


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
     * Copy constructor
     */
    WaitingRoom(WaitingRoom const &) = delete;

    /**
     * Assigment operator
     */
    WaitingRoom& operator=(WaitingRoom const &) = delete;

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

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "WaitingRoom starting" << commit;

    while (!boost::this_thread::interruption_requested()) {
        try {
            boost::this_thread::sleep(boost::posix_time::seconds(1));

            // lock the mutex
            boost::mutex::scoped_lock lock(me.m);
            // get current time
            time_t now = time(NULL);
            // iterate over all all tasks
            typename boost::ptr_list<TASK>::iterator it, next = me.tasks.begin();
            while ((it = next) != me.tasks.end()) {
                if (boost::this_thread::interruption_requested())
                    return;
                // next item to check
                ++next;
                // if the time has not yet come for the task simply continue
                if (it->waiting(now))
                    continue;
                // otherwise start the task
                me.pool->start(me.tasks.release(it).release());
            }
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "WaitingRoom interruption requested" << commit;
            break;
        }
        catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "WaitingRoom error: " << e.what() << commit;
        }
        catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "WaitingRoom unknown exception" << commit;
        }
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "WaitingRoom exiting" << commit;
}

#endif // WAITINGROOM_H_
