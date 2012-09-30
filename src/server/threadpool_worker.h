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

#pragma once

#include "server_dev.h"

#include "common/timeout.h"
#include "common/traced.h"
#include "threadtraits.h"
/* ---------------------------------------------------------------------- */

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

namespace ThreadPool {

class Worker 
{
public:
	Worker(ThreadTraits::THREAD_GROUP& tg, const int id);	

	virtual ~Worker() {};

private:
	void _doWork();
	ThreadTraits::MUTEX _mutex;
	
	
	static Timeout& _TIMEOUT() 
    {
		static InfiniteTimeout td;
		return td;
	}
	
	Traced<Worker, int> _tracer;
};

} // namespace ThreadPool

FTS3_SERVER_NAMESPACE_END
