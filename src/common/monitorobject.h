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

/** \file monitorobject.h Interface of MonitorObject class. */

#include <boost/thread.hpp>
#include "common_dev.h"

FTS3_COMMON_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/** \brief Implements the Monitor pattern that enables concurrent access
 * for the method of an object.
 *
 * The classes that need concurrent access
 * must derive from MonitorObject, and protect the method bodies with the
 * constructs provided by this class. Only those methods must be protected that
 * need synchronization. The best practice if the protection covers the entire
 * method body like the example:
 *
 * \code
 *
 * class MyClass : public Common::MonitorObject {
 *     int mySynchronizedMethod ()
 *     {
 *         FTS3_COMMON_MONITOR_START_CRITICAL
 *         // Do something
 *         return 0;
 *         FTS3_COMMON_MONITOR_END_CRITICAL
 *     }
 * };
 * \endcode
 *
 * Monitor pattern: http://en.wikipedia.org/wiki/Monitor_(synchronization)
 */
class MonitorObject
{
public:
    // Constructor (empty).
    MonitorObject() {};

    // Destructor (empty).
    virtual ~MonitorObject() {};

protected:
    // Lock used to protect a critical section. Do not use it directly, only with
    // FTS3_COMMON_MONITOR_* macros.
    mutable boost::mutex _monitor_lock;

    // Lock used to protect a static critical section (protects from all the objects
    // of a gven class). Do not use it directly, only with FTS3_COMMON_MONITOR_* macros.
    static boost::mutex& _static_monitor_lock()
    {
        static boost::mutex m;
        return m;
    }
};

/* -------------------------------------------------------------------------- */

/// The name of the lock variable that is set by FTS3_COMMON_MONITOR_START_CRITICAL.
#define FTS3_COMMON_MONITOR_LOCK __lock__

/// Opens a critical (protected) section. It must be closed by a corresponding FTS3_COMMON_MONITOR_END_CRITICAL.
/// If you fail to indicate the end of the critical section, you will get compilation error.
/// In runtime, the lock is always released at the end of the scope of the critical section.
#define FTS3_COMMON_MONITOR_START_CRITICAL { boost::mutex::scoped_lock FTS3_COMMON_MONITOR_LOCK(this->_monitor_lock);

/// Opens a "static" critical section (locks all the objects of a given class).
#define FTS3_COMMON_MONITOR_START_STATIC_CRITICAL { boost::mutex::scoped_lock FTS3_COMMON_MONITOR_LOCK(_static_monitor_lock());

/// Closes a critical (protected) section. Used for static sections as well.
#define FTS3_COMMON_MONITOR_END_CRITICAL }

FTS3_COMMON_NAMESPACE_END

