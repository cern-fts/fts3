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

#pragma once

#include <boost/bind.hpp>
#include <boost/utility.hpp>
#include <boost/thread/condition.hpp>
#include <assert.h>
#include "../common/MonitorObject.h"

namespace fts3 {
namespace server {

/* ---------------------------------------------------------------------- */

/** \brief Class implementing Active Object pattern.
 *
 * See the concept here: http://en.wikipedia.org/wiki/Active_object
 *
 */
template<
    class ExecutionPolicy /**< Determines how the system executes the method.
        Queues it, executes it immediately, etc. */
>
class ActiveObject :
    private boost::noncopyable,
    public fts3::common::MonitorObject
{
public:
    /* ------------------------------------------------------------------ */

    ActiveObject (const std::string& id, const std::string& description) :
        id(id), description(description), _runningMethods(0)
    {}

    /* ------------------------------------------------------------------ */

    virtual ~ActiveObject()
    {}

    /* ------------------------------------------------------------------ */
    std::string id;
    std::string description;


protected:

    /* ------------------------------------------------------------------ */

    typedef ActiveObject<ExecutionPolicy> BaseType;

    /* ------------------------------------------------------------------ */

    /** The method "body" itself, implementing the "Command" pattern */
    template<class OP>
    class Command
    {
    public:
        Command
        (
            ActiveObject* aobj,
            OP& op
        ) :
            _aobj(aobj),
            _op(op)
        {}

        /* -------------------------------------------------------------- */

        void operator () ()
        {
            _aobj->template _doWork(_op);
        }

    protected:

        /* -------------------------------------------------------------- */

        ActiveObject* _aobj;

        /* -------------------------------------------------------------- */

        OP _op;
    };

    /* ------------------------------------------------------------------ */

    template<class OP>
    void _enqueue(OP& op)
    {
        FTS3_COMMON_MONITOR_START_CRITICAL
        ++_runningMethods;
        FTS3_COMMON_MONITOR_END_CRITICAL
        Command<OP> packagedOp(this, op);
        ExecutionPolicy::instance().enqueue(packagedOp);
    }

    /* ------------------------------------------------------------------ */

    template<class OP>
    void _doWork(OP op)
    {
        try
            {
                op();
            }
        catch (...)
            {
                FTS3_COMMON_MONITOR_START_CRITICAL
                _decrease();
                throw;
                FTS3_COMMON_MONITOR_END_CRITICAL
            }

        FTS3_COMMON_MONITOR_START_CRITICAL
        _decrease();
        FTS3_COMMON_MONITOR_END_CRITICAL
    }

protected:

    /* ------------------------------------------------------------------ */

    void _decrease()
    {
        --_runningMethods;
        _notRunning.notify_all();
    }

    unsigned long _runningMethods;

    /* ------------------------------------------------------------------ */

    typename boost::condition _notRunning;
};

} // end namespace server
} // end namespace fts3

