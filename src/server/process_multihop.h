#pragma once

#include "process_service_multihop_handler.h"
#include "active_object.h"
#include "threadpool.h"
#include "common/traced.h"

FTS3_SERVER_NAMESPACE_START

/* -------------------------------------------------------------------------- */

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

class ProcessServiceMultihop;

struct ProcessServiceMultihopTraits
{
    typedef ActiveObject <ThreadPool::ThreadPool, Traced<ProcessServiceMultihop> > ActiveObjectType;
};

/* -------------------------------------------------------------------------- */

class ProcessServiceMultihop : public ProcessServiceMultihopHandler <ProcessServiceMultihopTraits>
{
public:
    ProcessServiceMultihop() {}

};

FTS3_SERVER_NAMESPACE_END

