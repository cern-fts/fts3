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

#include "process_service_handler.h"

#include "active_object.h"
#include "threadpool.h"
#include "common/traced.h"


FTS3_SERVER_NAMESPACE_START

/* -------------------------------------------------------------------------- */

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

class ProcessService;

struct ProcessServiceTraits
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<ProcessService> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class ProcessService : public ProcessServiceHandler <ProcessServiceTraits>
{
public:
    ProcessService() {}

};

FTS3_SERVER_NAMESPACE_END

