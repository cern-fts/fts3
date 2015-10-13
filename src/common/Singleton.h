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

#ifndef SINGLETON_H_
#define SINGLETON_H_

#include <memory>
#include <mutex>


namespace fts3
{
namespace common
{

/**
 * The Singleton ensures a thread safe access to the hold instance.
 *  Note that it grands access to the same instance as the InstanceHolder class!
 *
 * It might be used as a base class for a thread-safe singleton.
 *
 * @see InstanceHolder, MonitorObject
 */
template <typename T>
class Singleton
{
public:

    /// Gets a references to T, is thread safe.
    static T& instance()
    {
        // thread safe lazy loading
        if (Singleton<T>::getInstancePtr().get() == NULL)
        {
            std::lock_guard<std::mutex> lock(Singleton<T>::getMutex());
            if (Singleton<T>::getInstancePtr().get() == 0)
            {
                Singleton<T>::getInstancePtr().reset(new T);
            }
        }
        return *Singleton<T>::getInstancePtr();
    }

    /// Destroy the singleton
    static void destroy()
    {
        std::lock_guard<std::mutex> lock(Singleton<T>::getMutex());
        Singleton<T>::getInstancePtr().reset(NULL);
    }

protected:
    /// Constructor
    Singleton() {}
    virtual ~Singleton() {};

private:
    /// Instance
    /// Putting it inside a method puts the instantiation and uniqueness on the compiler
    static std::unique_ptr<T>& getInstancePtr() {
        static std::unique_ptr<T> instancePtr;
        return instancePtr;
    }

    /// Mutex for thread-safe instantiation
    static std::mutex& getMutex() {
        static std::mutex mutex;
        return mutex;
    }

    /// Copying constructor.
    /// Private, should not be used
    Singleton(const Singleton&) = delete;

    /// Assignment operator.
    /// Private, should not be used
    Singleton& operator=(Singleton const&) = delete;
};

}
}

#endif // SINGLETON_H_
