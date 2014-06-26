/*
 * WaitingRoom.h
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#ifndef WAITINGROOM_H_
#define WAITINGROOM_H_

#include "PollTask.h"

#include "common/ThreadPool.h"

#include <boost/ptr_container/ptr_list.hpp> // thing about using a lockfree queue

#include <boost/thread.hpp>
#include <boost/scoped_ptr.hpp>

using namespace fts3::common;

class WaitingRoom
{

public:

    static WaitingRoom& get()
    {
        static WaitingRoom instance;
        return instance;
    }

    void add(StagingTask* task)
    {
        boost::mutex::scoped_lock lock(m);
        tasks.push_back(task);
    }

    void attach(ThreadPool<StagingTask>& pool)
    {
        this->pool = &pool;
        // now we can start the thread
        t.reset(new boost::thread(run));
    }

    virtual ~WaitingRoom() {}

private:

    static void run();

    WaitingRoom() : t(0), pool(0) {}
    WaitingRoom(WaitingRoom const &);
    WaitingRoom& operator=(WaitingRoom const &);

    /// the queue with tasks that are waiting
    boost::ptr_list<StagingTask> tasks;
    /// the mutex preventing concurrent access
    boost::mutex m;
    /// the worker thread
    boost::scoped_ptr<boost::thread> t;
    /// the threadpool items are waiting for
    ThreadPool<StagingTask> * pool;
};

#endif /* WAITINGROOM_H_ */
