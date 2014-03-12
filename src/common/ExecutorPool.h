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
 * ExecutorPool.h
 *
 *  Created on: Dec 9, 2013
 *      Author: Michał Simon
 */

#ifndef EXECUTORPOOL_H_
#define EXECUTORPOOL_H_

#include "common/Executor.h"

#include <algorithm>

#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace fts3
{
namespace common
{

using namespace boost;

template <typename T>
class ExecutorPool
{


public:

    ExecutorPool(int size) : index(0), size(size), noexec(0) {}

    virtual ~ExecutorPool() {}

    /**
     * Adds new TransferFile to the queue of one of the FileTransferExecutors
     *
     * @param tf - the TransferFile to be carried out
     * @param optimize - should be true if autotuning should be used for this FileTransfer
     */
    void add(T* t)
    {
        // if needed add new worker loop
        if (executors.size() < size)
            {
                Executor<T>* exec = new Executor<T>();
                executors.push_back(exec);
                group.create_thread(bind(&Executor<T>::execute, exec));
            }
        // add the task to worker's queue
        executors[index].put(t);
        index++;
        if(index == size) index = 0;
    }

    /**
     * Waits until all the FileTransferExecutors in the pool finish their job
     */
    void join()
    {
        // notify each executor that there are no more data
        for_each(executors.begin(), executors.end(), bind(&Executor<T>::noMoreData, _1));
        // wait until all threads terminate
        group.join_all();
        // sum up the number of successfully executed elements
        for_each(executors.begin(), executors.end(), bind(&ExecutorPool<T>::sum, this, _1));
    }

    /**
     * Stops all the threads in the pool
     */
    void stop()
    {
        group.interrupt_all();
    }

    int executed()
    {
        return noexec;
    }

private:

    void sum(Executor<T>& exec)
    {
        noexec += exec.executed();
    }

    /// vector with executors
    ptr_vector< Executor<T> > executors;

    // group with executors threads
    thread_group group;

    /// the next queue (executor) to be used
    unsigned int index;
    /// size of the pool
    unsigned int size;
    /// number of elements that were executed successfully (e.g. scheduled)
    int noexec;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* EXECUTORPOOL_H_ */
