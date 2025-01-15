/*
 * Copyright (c) CERN 2013-2025
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

#include <queue>
#include <condition_variable>
#include <mutex>
#include <cassert>
#include "common/Logger.h"

namespace fts3 {
namespace common {

template<typename T>
class ConcurrentQueue {
private:
    std::queue<T> theQueue;

    std::mutex mutex;
    std::condition_variable_any cv;
    std::stop_token stop_token;

    ConcurrentQueue() {};

public:
    static const size_t MaxElements = 20000;

    static ConcurrentQueue<T>& getInstance()
    {
        static ConcurrentQueue<T> instance;
        return instance;
    }

    void set_stop_token(std::stop_token token)
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop_token = token;
    }

    /// Return true if the queue is empty
    bool empty()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return theQueue.empty();
    }

    /// Return the size of the queue
    size_t size()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return theQueue.size();
    }

    /// Push a new element into the queue
    void push(const T &value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (theQueue.size() < ConcurrentQueue::MaxElements)
            theQueue.push(value);
        lock.unlock();

        cv.notify_all();
    }

    void push(T &&value)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (theQueue.size() < ConcurrentQueue::MaxElements)
            theQueue.push(std::move(value));
        lock.unlock();

        cv.notify_all();
    }


    /// Pop an element off the queue
    /// If wait is 0, return null if the queue is empty, otherwise wait until an item is placed in the queue
    /// Potentially in the future:
    ///  wait = -1 => block until an item is placed in the queue, and return it
    ///  wait = 0  => don't block, return null if the queue is empty
    /// wait > 0  => block for <block> seconds
    T pop(const int wait = -1)
    {
        std::unique_lock<std::mutex> lock(mutex);

        if (theQueue.empty()) {
            if (wait == 0) {
                return T();
            } else if (wait > 0) {
                cv.wait_for(lock, stop_token, std::chrono::seconds(wait), [this] { return !theQueue.empty(); });
            } else if (wait == -1) {
                cv.wait(lock, stop_token, [this] {return !theQueue.empty();});
            }

            if (theQueue.empty()) {
                return T();
            }
        }

        T ret(std::move(theQueue.front()));
        theQueue.pop();
        lock.unlock();

        cv.notify_all();
        return ret;
    }

    void drain()
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (theQueue.empty()) {
            return;
        }

        const size_t queue_size = theQueue.size();
        cv.wait(lock, stop_token, [this, &queue_size] {return theQueue.size() < queue_size;});
    }
};


}
}
