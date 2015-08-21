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

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include "server_dev.h"

FTS3_SERVER_NAMESPACE_START

using namespace FTS3_COMMON_NAMESPACE;

/* ---------------------------------------------------------------------- */

class ThreadFunction
{
private:
    /* ------------------------------------------------------------------ */

    class ProgressCondition
    {
    public:
        ProgressCondition()
            : _stopped(false)
        {};

        /* -------------------------------------------------------------- */

        bool wait(const boost::xtime t)
        {
            boost::recursive_mutex::scoped_lock lock(_mutex);
            bool ret = _stopped ? true : _cond.timed_wait(lock, t);
            return ret;
        };

        /* -------------------------------------------------------------- */

        void finished()
        {
            boost::recursive_mutex::scoped_lock lock(_mutex);
            _stopped = true;
            _cond.notify_all();
        }

    private:

        /* -------------------------------------------------------------- */

        bool _stopped;

        /* -------------------------------------------------------------- */

        mutable boost::recursive_mutex _mutex;

        /* -------------------------------------------------------------- */

        boost::condition _cond;
    };

public:
    ThreadFunction() : _pcond(new ProgressCondition) {};
    ThreadFunction(const ThreadFunction& tf) : _pcond(tf._pcond) {};
    virtual ~ThreadFunction() {};

    bool wait(const boost::xtime t)
    {
        return _pcond->wait(t);
    };

protected:
    void _finished()
    {
        _pcond->finished();
    }

    virtual void operator () () = 0;

private:
    std::shared_ptr<ProgressCondition> _pcond;
};

FTS3_SERVER_NAMESPACE_END

