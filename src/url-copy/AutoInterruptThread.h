/*
 * Copyright (c) CERN 2016
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

#ifndef AUTOINTERRUPT_THREAD_H
#define AUTOINTERRUPT_THREAD_H

#include <boost/function.hpp>
#include <boost/thread.hpp>

/// Automatically interrupt and wait for the thread on destruction
class AutoInterruptThread {
private:
    boost::thread thread;

public:
    AutoInterruptThread(boost::function<void()> func): thread(func) {
    }

    ~AutoInterruptThread() {
        thread.interrupt();
        thread.join();
    }
};


#endif // AUTOINTERRUPT_THREAD_H
