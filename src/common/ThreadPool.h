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

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>

namespace fts3
{
namespace common
{

/**
 * A generic thread-pool class
 */
template <typename TASK, typename INIT_FUNC = void (*)(boost::any&)>
class ThreadPool
{
    /// optional initialisation function (a typedef for convenience)
    typedef boost::optional<INIT_FUNC> init_func;

    /**
     * A helper class that retrieves subsequent tasks from the queue
     * and then executes them
     */
    struct ThreadPoolWorker
    {
    public:

        /// constructor
        ThreadPoolWorker(ThreadPool & pool, init_func init_context) : t_pool(pool)
        {
            if (init_context.is_initialized()) {
                (*init_context)(thread_context);
            }
        }

        /// the run routine that retrieves subsequent tasks
        /// from the queue and then executes them
        void run()
        {
            while (!t_pool.interrupt_flag) {
                std::unique_ptr<TASK> task(t_pool.next());
                if (!task.get()) break;
                task->run(thread_context);
            }
        }

        /// optional data, initialised by the 'init_context' parameter
        boost::any thread_context;
        /// reference to the thread pool object
        ThreadPool & t_pool;
    };

public:

    /**
     * constructor
     *
     * @param size : size of the thread pool
     */
    ThreadPool(int size, init_func init_context = init_func()) : workers(size), interrupt_flag(false), join_flag(false)
    {
        for (int i = 0; i < size; ++i) {
            // create new worker
            ThreadPoolWorker *worker = new ThreadPoolWorker(*this, init_context);
            // take ownership of the memory
            workers.push_back(worker);
            // create new thread belonging to the right group
            group.create_thread(boost::bind(&ThreadPoolWorker::run, worker));
        }
    }

    /// destructor
    virtual ~ThreadPool()
    {
        interrupt();
        join();
    }

    /**
     * Executes a task.
     *
     * Please note that the thread-pool takes ownership of the pointer!
     *
     * @param t : task that will be executed
     */
    void start(TASK *t)
    {
        {
            // lock the queue
            boost::mutex::scoped_lock lock(mx);
            tasks.push_back(t);
        }
        // notify waiting threads
        cvar.notify_all();
    }

    /// interrupt all the threads belonging to this thread pool
    void interrupt()
    {
        interrupt_flag = true;
        group.interrupt_all();
    }

    /// join all the threads within this thread pool
    void join()
    {
        // lock the queue
        {
            boost::mutex::scoped_lock lock(mx);
            join_flag = true;
        }
        // notify waiting threads
        cvar.notify_all();
        // join ...
        group.join_all();
    }

    /// @return size of the thread pool
    size_t size()
    {
        return group.size();
    }

    /**
     * Executes a reduce operation on all thread contexts
     *
     * @param RET : return type of the operation
     * @param OPERATION : functional object template
     * @param op : functional object providing the reduce operation
     *             (e.g. std::plus, std::minus, etc.)
     * @return : the result of applying the operation to all contexts
     *           (e.g. a sum if std::plus is passed as a parameter)
     */
    template<class RET, template<class> class OPERATION>
    RET reduce(OPERATION<RET> op)
    {
        typename boost::ptr_vector<ThreadPoolWorker>::iterator it;
        RET init = RET();

        for (it = workers.begin(); it != workers.end(); ++it) {
            if (it->thread_context.empty()) continue;
            init = op(init, boost::any_cast<RET>(it->thread_context));
        }

        return init;
    }

private:

    /**
     * Returns the next task in the queue
     *
     * @return next task in the queue
     */
    TASK* next()
    {
        // lock the queue
        boost::mutex::scoped_lock lock(mx);
        // if the queue is empty wait until someone puts something inside
        // (unless the join flag is raised)
        while (tasks.empty() && !join_flag) {
            cvar.wait(lock);
        }
        // take the first element from the queue
        typename boost::ptr_deque<TASK>::iterator it = tasks.begin();
        // if the queue is empty return null
        if (it == tasks.end()) {
            return 0;
        }
        // otherwise give up ownership of the object and return it
        return tasks.release(it).release();
    }

    /// group with worker threads
    boost::thread_group group;
    /// the mutex preventing access
    boost::mutex mx;
    /// conditional variable preventing concurrent browsing and reconnecting
    boost::condition_variable cvar;
    /// the queue itself
    boost::ptr_deque<TASK> tasks;
    /// pool of worker objects
    boost::ptr_vector<ThreadPoolWorker> workers;
    /// a flag indicating whether all threads should be stopped
    bool interrupt_flag;
    /// a flag indicating whether someone is joining us
    bool join_flag;
};

} /* namespace common */
} /* namespace fts3 */

#endif /* THREADPOOL_H_ */
