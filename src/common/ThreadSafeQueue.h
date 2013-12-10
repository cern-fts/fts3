/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ThreadSafeQueue.h
 *
 *  Created on: Aug 9, 2013
 *      Author: simonmMichał Simon
 */

#ifndef THREADSAFEQUEUE_H_
#define THREADSAFEQUEUE_H_

#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_list.hpp>

namespace fts3
{

namespace common
{

/**
 * A thread safe queue, allows only for putting and popping
 *
 * An element is a pointer to the type specified by the template parameter
 *
 * If empty is called and queue is empty but 'no_more_date' flag is set to false
 * blocks until some element will be inserted
 *
 * Handles the memory deallocation for user.
 */
template<typename T>
class ThreadSafeQueue
{

public:

	/// Constructor, sets no_more_data to false
    ThreadSafeQueue() : no_more_data(false) {}

    /// Destructor
    virtual ~ThreadSafeQueue() {}

    /**
     * Insert an element
     */
    void push(T* e)
    {
    	{
    		boost::mutex::scoped_lock lock(qm);
    		ts_queue.push_back(e);
    	}
        qv.notify_all();
    }

    /**
     * Get a reference to the element in front
     */
    T& front()
    {
    	boost::mutex::scoped_lock lock(qm);
    	return ts_queue.front();
    }

    /**
     * Remove the element in front from queue (this deallocates the memory)
     */
    void pop()
    {
    	boost::mutex::scoped_lock lock(qm);
    	ts_queue.pop_front();
    }

    /**
     * Check if the queue is empty (blocks if there is no data in the queue and no_more_data is set to false)
     */
    bool empty()
    {
        boost::mutex::scoped_lock lock(qm);
        while (ts_queue.empty() && !no_more_data) qv.wait(lock);
        return ts_queue.empty();
    }

    /**
     * Sets no_more_data to ture
     */
    void noMoreData ()
    {
    	{
    		boost::mutex::scoped_lock lock(qm);
    		no_more_data = true;
    	}
        qv.notify_all();
    }

private:

    /// the queue itself
    boost::ptr_list<T> ts_queue;

    /// the mutex preventing concurrent browsing and reconnecting
    boost::mutex qm;

    /// conditional variable preventing concurrent browsing and reconnecting
    boost::condition_variable qv;

    /// set to true if no more data are intended to be put into the queue
    bool no_more_data;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* THREADSAFEQUEUE_H_ */
