/*
 * process_reuse_handler.h
 *
 *  Created on: 1 Sep 2014
 *      Author: simonm
 */

#pragma once

#include "process_service_reuse_handler.h"
#include "active_object.h"
#include "threadpool.h"
#include "common/traced.h"

FTS3_SERVER_NAMESPACE_START

/* -------------------------------------------------------------------------- */

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

class ProcessServiceReuse;

struct ProcessServiceReuseTraits
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<ProcessServiceReuse> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class ProcessServiceReuse : public ProcessServiceReuseHandler <ProcessServiceReuseTraits>
{
public:
    ProcessServiceReuse() {}

};

FTS3_SERVER_NAMESPACE_END

