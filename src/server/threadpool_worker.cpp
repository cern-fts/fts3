/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License"); 
you may not use this file except in compliance with the License. 
You may obtain a copy of the License at 

    http://www.apache.org/licenses/LICENSE-2.0 

Unless required by applicable law or agreed to in writing, software 
distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and 
limitations under the License. */

#include "threadpool_worker.h"
#include "threadpool.h"
#include "common/logger.h"
#include "StaticSslLocking.h"

extern bool  stopThreads;

FTS3_SERVER_NAMESPACE_START

namespace ThreadPool {

using namespace FTS3_COMMON_NAMESPACE;

/* ---------------------------------------------------------------------- */

Worker::Worker(ThreadTraits::THREAD_GROUP& tg, const int id) 
	: _tracer("ThreadPoolWorker", id)
{	
	tg.create_thread(boost::bind(&Worker::_doWork, this));		
}

/* ---------------------------------------------------------------------- */

void Worker::_doWork() 
{
	
	while(1) {
          if(stopThreads)
                return;

		_TIMEOUT().actualize();
		ThreadPool::element_type task(ThreadPool::instance().pop(_TIMEOUT()));	

		if (task.get() != NULL) 
        {
	    if(stopThreads) break;
            task->execute();
			
	}
	}
	

}

} // namespace ThreadPool

FTS3_SERVER_NAMESPACE_END

