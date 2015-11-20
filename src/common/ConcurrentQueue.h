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

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <queue>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

namespace fts3 {
namespace common {


class ConcurrentQueue {
private:
    static ConcurrentQueue *single;
    boost::mutex mutex;
    boost::condition_variable cv;

    ConcurrentQueue();

public:
    static const size_t MaxElements = 20000;

    static ConcurrentQueue *getInstance();

    std::queue<std::string> theQueue;

    /// Return true if the queue is empty
    bool empty();

    /// Return the size of the queue
    size_t size();

    /// Push a new element into the queue
    void push(const std::string &value);

    /// Pop an element off the queue
    /// If wait is 0, return null if the queue is empty, otherwise wait until an item is placed in the queue
    /// Potentially in the future:
    ///  wait = -1 => block until an item is placed in the queue, and return it
    ///  wait = 0  => don't block, return null if the queue is empty
    /// wait > 0  => block for <block> seconds
    std::string pop(const int wait = -1);
};


}
}

#endif
