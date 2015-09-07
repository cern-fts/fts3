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

#ifndef THREADSAFESINGLETON_H_
#define THREADSAFESINGLETON_H_

#include "common/InstanceHolder.h"
#include "MonitorObject.h"

namespace fts3
{
namespace common
{

/**
 * The ThreadSafeInstanceHolder ensures a thread safe access to the hold instance.
 * 	Note that it grands access to the same instance as the InstanceHolder class!
 *
 * It might be used as a base class for a thread-safe singleton.
 *
 * @see InstanceHolder, MonitorObject
 */
template <typename T>
class ThreadSafeInstanceHolder: public MonitorObject, public InstanceHolder<T>
{

public:

    /**
     * Gets a references to T, is thread safe.
     *
     * @return reference to T
     */
    static T& getInstance()
    {
        // thread safe lazy loading
        if (InstanceHolder<T>::instance.get() == 0)
            {
                FTS3_COMMON_MONITOR_START_STATIC_CRITICAL
                if (InstanceHolder<T>::instance.get() == 0)
                    {
                        InstanceHolder<T>::instance.reset(new T);
                    }
                FTS3_COMMON_MONITOR_END_CRITICAL
            }
        return *InstanceHolder<T>::instance;
    }


    /**
     * Destructor (empty).
     */
    virtual ~ThreadSafeInstanceHolder() {};

protected:

    /**
     * Default constructor (empty).
     *
     * Private
     */
    ThreadSafeInstanceHolder() {};

private:
    /**
     * Copying constructor.
     *
     * Private, should not be used
     */
    ThreadSafeInstanceHolder(ThreadSafeInstanceHolder const&);

    /**
     * Assignment operator.
     *
     * Private, should not be used
     */
    ThreadSafeInstanceHolder& operator=(ThreadSafeInstanceHolder const&);
};

}
}

#endif /* THREADSAFESINGLETON_H_ */
