/*
 * ThreadPool.h
 *
 *  Created on: 23 May 2014
 *      Author: simonm
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include "ThreadSafeQueue.h"

#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_deque.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
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
    class ThreadPoolWorker
    {

    public:

        /// constructor
        ThreadPoolWorker(ThreadPool & pool, init_func init_context) : t_pool(pool)
        {
            if (init_context.is_initialized()) (*init_context)(thread_context);
        }

        /// the run routine that retrieves subsequent tasks
        /// from the queue and then executes them
        void run()
        {
            while(!t_pool.interrupt_flag)
                {
                    boost::scoped_ptr<TASK> task(t_pool.next());
                    if (!task.get()) break;
                    task->run(thread_context);
                }
        }

    private:
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
        for (int i = 0; i < size; ++i)
            {
                // create new worker
                ThreadPoolWorker* worker = new ThreadPoolWorker(*this, init_context);
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
    }

    /**
     * Execute a task
     *
     * @param t : task that will be executed
     */
    void start(TASK* t)
    {
        {
            // lock the queue
            boost::mutex::scoped_lock lock(qm);
            tasks.push_back(t);
        }
        // notify waiting threads
        qv.notify_all();
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
            boost::mutex::scoped_lock lock(qm);
            join_flag = true;
        }
        // notify waiting threads
        qv.notify_all();
        // join ...
        group.join_all();
    }

    /// @return size of the thread pool
    size_t size()
    {
        return group.size();
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
        boost::mutex::scoped_lock lock(qm);
        // if the queue is empty wait until someone puts something inside
        // (unless the join flag is raised)
        while (tasks.empty() && !join_flag) qv.wait(lock);
        // take the first element from the queue
        typename boost::ptr_deque<TASK>::iterator it = tasks.begin();
        // if the queue is empty return null
        if (it == tasks.end()) return 0;
        // otherwise give up ownership of the object and return it
        return tasks.release(it).release();
    }

    /// group with worker threads
    boost::thread_group group;
    /// the mutex preventing access
    boost::mutex qm;
    /// conditional variable preventing concurrent browsing and reconnecting
    boost::condition_variable qv;
    /// the queue itsef
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
