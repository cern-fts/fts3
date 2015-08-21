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

#include "threadpool_worker.h"
#include "threadpool.h"
#include "common/logger.h"

extern bool  stopThreads;

namespace fts3 {
namespace server {
namespace ThreadPool
{

using namespace fts3::common;

/* ---------------------------------------------------------------------- */

Worker::Worker(boost::thread_group& tg, const int id)
{
    thr = tg.create_thread(boost::bind(&Worker::_doWork, this));
}


void Worker::cancel()
{
    boost::thread::native_handle_type hnd=thr->native_handle();
    try
        {
            if(hnd)
                pthread_cancel(hnd);
        }
    catch(...)
        {
            throw;
        }
}

/* ---------------------------------------------------------------------- */

void Worker::_doWork()
{
    try
        {
            while(stopThreads==false)
                {
                    _TIMEOUT().actualize();
                    ThreadPool::element_type task(ThreadPool::instance().pop(_TIMEOUT()));

                    if (task.get() != NULL)
                        {
                            task->execute();

                        }
                }
        }
    catch(...)
        {
            //throw;
        }

}

} // namespace ThreadPool
} // end namespace server
} // end namespace fts3

