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
 * Executor.h
 *
 *  Created on: Dec 9, 2013
 *      Author: Michał Simon
 */

#ifndef EXECUTOR_H_
#define EXECUTOR_H_

#include "common/ThreadSafeQueue.h"

#include <boost/thread.hpp>

namespace fts3
{

namespace common
{

using namespace boost;

template <typename T>
class Executor
{

public:

	/**
	 * Constructor
	 */
	Executor() : active(true), noexec(0) {}

	/**
	 * Destructor
	 */
	virtual ~Executor()
	{
		active = false;
	}

    /**
     * Worker loop that is executed in a separate thread
     */
    void execute()
    {
        while (active && !queue.empty() && !stopThreads)
            {
				// execute get the element in front
				noexec += queue.front().execute();
				// remove the element from queue (it also deallocates the memory)
				queue.pop();
            }

        // the thread is not active anymore
        active = false;
    }

    /**
     * Add new element to the queue
     */
    void put(T* t)
    {
        queue.push(t);
    }

    /**
     * Calling this method indicates that no more data will be pushed into the queue
     */
    void noMoreData()
    {
        queue.noMoreData();
    }

    /**
     * @return true if the worker loop is active, fale otherwise
     */
    bool isActive() const
    {
        return active;
    }

    int executed() const
    {
    	return noexec;
    }

private:

    /// queue with all the tasks
    ThreadSafeQueue<T> queue;

    /// state of the worker
    bool active;

    /// number of elements from the queue that were executed successfully (e.g. were scheduled)
    /// this is not synchronized so it can be only read when all elements were processed and the thread terminated
    int noexec;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* EXECUTOR_H_ */
