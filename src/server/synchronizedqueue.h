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

#include "common/timeout.h"
#include "common/logger.h"
#include "common/error.h"
#include "common/monitorobject.h"

#include <limits>
#include <deque>
#include <boost/thread/condition.hpp>


namespace fts3 {
namespace server {

/**
 * Ref: An Object Behavioral Pattern for Concurrent Programming, Douglas C. Schmidt
 */
template
<
class ELEMENT,
      template <class> class ELEMENT_ACCESS
      >
class SynchronizedQueue : public fts3::common::MonitorObject
{
public:
    /* ---------------------------------------------------------------------- */

    typedef ELEMENT_ACCESS<ELEMENT> element_type;

    /* ---------------------------------------------------------------------- */

    SynchronizedQueue
    (
        size_t maxSize = std::numeric_limits<size_t>::max()
    )
        : _maxSize(maxSize)
    {};

    /* ---------------------------------------------------------------------- */

    virtual ~SynchronizedQueue()
    {
        _notEmpty.notify_all();
        _notFull.notify_all();
    };

    /* ---------------------------------------------------------------------- */

    void push(const element_type& e)
    {
        FTS3_COMMON_MONITOR_START_CRITICAL
        // Wait while the queue is full
        while (_full())
            {
                _notFull.wait(FTS3_COMMON_MONITOR_LOCK);
            }

        _push(e);
        _notEmpty.notify_all();
        FTS3_COMMON_MONITOR_END_CRITICAL
    };

    /* ---------------------------------------------------------------------- */

    element_type pop(const fts3::common::Timeout& tdiff)
    {
        bool isNotTimeout = true;
        FTS3_COMMON_MONITOR_START_CRITICAL
        // Wait while the queue is empty
        while (_empty() && isNotTimeout)
            {
                isNotTimeout = _notEmpty.timed_wait(FTS3_COMMON_MONITOR_LOCK, tdiff.getXtime());
            }

        element_type e = (isNotTimeout ? _pop() : element_type());
        _notFull.notify_all();

        return e;
        FTS3_COMMON_MONITOR_END_CRITICAL
    };

    /* ---------------------------------------------------------------------- */

    element_type pop()
    {
        FTS3_COMMON_MONITOR_START_CRITICAL
        // Wait while the queue is empty
        while (_empty())
            {
                _notEmpty.wait(FTS3_COMMON_MONITOR_LOCK);
            }

        element_type e = _pop();
        _notFull.notify_all();
        return e;
        FTS3_COMMON_MONITOR_END_CRITICAL
    };

    /* ---------------------------------------------------------------------- */

    bool empty() const
    {
        FTS3_COMMON_MONITOR_START_CRITICAL
        return _empty();
        FTS3_COMMON_MONITOR_END_CRITICAL
    };

    /* ---------------------------------------------------------------------- */

    bool full() const
    {
        FTS3_COMMON_MONITOR_START_CRITICAL
        return _full();
        FTS3_COMMON_MONITOR_END_CRITICAL
    }

private:

    /* ---------------------------------------------------------------------- */

    bool _empty() const
    {
        return _queue.empty();
    }

    /* ---------------------------------------------------------------------- */

    bool _full() const
    {
        return _queue.size() >= _maxSize;
    }

    /* ---------------------------------------------------------------------- */

    void _push(const element_type& e)
    {
        assert(!_full());
        _queue.push_back(e);
    }

    /* ---------------------------------------------------------------------- */

    element_type _pop()
    {
        assert(!_empty());
        element_type e = _queue.front();
        _queue.pop_front();
        return e;
    }

    /* ---------------------------------------------------------------------- */

    const size_t _maxSize;

    /* ---------------------------------------------------------------------- */

    std::deque<element_type> _queue;
    /* ---------------------------------------------------------------------- */

    boost::condition _notEmpty;

    /* ---------------------------------------------------------------------- */

    boost::condition _notFull;
};

} // namespace server
} // namespace fts3

