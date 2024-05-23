/*
 * Copyright (c) CERN 2024
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

#include <gtest/gtest.h>
#include <boost/chrono.hpp>

#include "common/ConcurrentQueue.h"

using fts3::common::ConcurrentQueue;


TEST(ConcurrentQueueTEST, Simple) {
    ConcurrentQueue *queue = ConcurrentQueue::getInstance();
    EXPECT_EQ(queue->empty(), true);
    queue->push("abcde");
    queue->push("cdefg");
    EXPECT_EQ(queue->size(), 2);
    std::string str = queue->pop();
    EXPECT_EQ(str, "abcde");
    EXPECT_EQ(queue->size(), 1);
    EXPECT_EQ(queue->empty(), false);
    queue->pop();
    EXPECT_EQ(queue->empty(), true);
    EXPECT_EQ(queue->size(), 0);
}


TEST(ConcurrentQueueTEST, Blocking) {
    boost::chrono::steady_clock::time_point start, end;

    ConcurrentQueue *queue = ConcurrentQueue::getInstance();
    queue->push("abcde");

    start = boost::chrono::steady_clock::now();
    std::string str = queue->pop(5);
    end = boost::chrono::steady_clock::now();
    EXPECT_LT((end - start), boost::chrono::seconds(2));
    EXPECT_EQ(str, "abcde");

    start = boost::chrono::steady_clock::now();
    str = queue->pop(5);
    end = boost::chrono::steady_clock::now();

    // Boost CV implementation does not use a monotonic clock, so be generous
    EXPECT_GE((end - start), boost::chrono::seconds(3));
    EXPECT_EQ(str, "");
}
