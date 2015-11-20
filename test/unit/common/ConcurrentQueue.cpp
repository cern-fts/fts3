/*
 * Copyright (c) CERN 2015
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

#include <boost/test/included/unit_test.hpp>

#include "common/ConcurrentQueue.h"

using fts3::common::ConcurrentQueue;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(ConcurrentQueueTest)


BOOST_AUTO_TEST_CASE (simple)
{
    ConcurrentQueue *queue = ConcurrentQueue::getInstance();
    BOOST_CHECK_EQUAL(queue->empty(), true);
    queue->push("abcde");
    queue->push("cdefg");
    BOOST_CHECK_EQUAL(queue->size(), 2);
    std::string str = queue->pop();
    BOOST_CHECK_EQUAL(str, "abcde");
    BOOST_CHECK_EQUAL(queue->size(), 1);
    BOOST_CHECK_EQUAL(queue->empty(), false);
    queue->pop();
    BOOST_CHECK_EQUAL(queue->empty(), true);
    BOOST_CHECK_EQUAL(queue->size(), 0);
}


BOOST_AUTO_TEST_CASE (blocking)
{
    time_t start, end;
    ConcurrentQueue *queue = ConcurrentQueue::getInstance();
    queue->push("abcde");

    start = time(NULL);
    std::string str = queue->pop(5);
    end = time(NULL);
    BOOST_CHECK_LT((end - start), 2);
    BOOST_CHECK_EQUAL(str, "abcde");

    start = time(NULL);
    str = queue->pop(3);
    end = time(NULL);

    BOOST_CHECK_GE((end - start), 3);
    BOOST_CHECK_EQUAL(str, "");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
