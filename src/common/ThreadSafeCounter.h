/*
 * Copyright (c) CERN 2022
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

#include "shared_mutex"

class ThreadSafeCounter
{
public:
    ThreadSafeCounter() = default;

    ThreadSafeCounter(const ThreadSafeCounter& copy) = delete;

    ThreadSafeCounter(ThreadSafeCounter&& copy) = delete;

    void increment() {
        std::unique_lock lock(mx);
        counter++;
    }

    void decrement() {
        std::unique_lock lock(mx);
        counter--;
    }

    uint64_t getCounter() const {
        std::shared_lock lock(mx);
        return counter;
    }

private:
    uint64_t counter = 0;
    // prevents concurrent access to counter
    mutable std::shared_mutex mx;
};

