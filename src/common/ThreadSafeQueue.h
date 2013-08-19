/*
 * ThreadSafeQueue.h
 *
 *  Created on: Aug 9, 2013
 *      Author: simonm
 */

#ifndef THREADSAFEQUEUE_H_
#define THREADSAFEQUEUE_H_

#include <queue>
#include <boost/thread.hpp>
#include <iostream>

namespace fts3
{

namespace common
{

using namespace std;
using namespace boost;

/**
 * A thread safe queue, allows only for putting and getting
 *
 * An element is a pointer to the type specified by the template parameter
 *
 * If get called and queue is empty blocks until some element will be inserted.
 * If no_more_data is set to true wont block if there are no data in the queue.
 */
template<typename T>
class ThreadSafeQueue
{

public:
    ThreadSafeQueue() : no_more_data(false)
    {

    }

    virtual ~ThreadSafeQueue()
    {

    }

    /**
     * Insert element
     */
    void put(T* e)
    {
        mutex::scoped_lock lock(qm);
        ts_queue.push(e);
        qv.notify_all();
    }

    /**
     * Get next element
     */
    T* get()
    {
        // get the mutex
        mutex::scoped_lock lock(qm);
        // if the queue is empty ...
        while (ts_queue.empty())
            {
                // if no data will be put into the queue ever return 0
                if (no_more_data) return 0;
                // otherwise wait
                qv.wait(lock);
            }
        // get an item from the queue
        T* e = ts_queue.front();
        ts_queue.pop();
        return e;
    }

    bool hasData()
    {
        mutex::scoped_lock lock(qm);
        return !ts_queue.empty() || !no_more_data;
    }

    void noMoreData ()
    {
        mutex::scoped_lock lock(qm);
        no_more_data = true;
        qv.notify_all();
    }

private:
    /// the queue itself

    queue<T*> ts_queue;

    /// the mutex preventing concurrent browsing and reconnecting
    mutex qm;

    /// conditional variable preventing concurrent browsing and reconnecting
    condition_variable qv;

    /// set to true if no more data are intended to be put into the queue
    bool no_more_data;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* THREADSAFEQUEUE_H_ */
