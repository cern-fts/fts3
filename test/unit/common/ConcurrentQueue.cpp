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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "common/ConcurrentQueue.h"

using fts3::common::ConcurrentQueue;

namespace std {
    std::ostream &operator<<(std::ostream &os, const boost::posix_time::time_duration &sec)
    {
        os << sec.total_milliseconds() << "ms";
        return os;
    }
}


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
    boost::posix_time::ptime start, end;

    ConcurrentQueue *queue = ConcurrentQueue::getInstance();
    queue->push("abcde");

    start = boost::posix_time::microsec_clock::universal_time();
    std::string str = queue->pop(5);
    end = boost::posix_time::microsec_clock::universal_time();
    BOOST_CHECK_LT((end - start), boost::posix_time::seconds(2));
    BOOST_CHECK_EQUAL(str, "abcde");

    start = boost::posix_time::microsec_clock::universal_time();
    str = queue->pop(3);
    end = boost::posix_time::microsec_clock::universal_time();

    BOOST_CHECK_GE((end - start), boost::posix_time::seconds(3));
    BOOST_CHECK_EQUAL(str, "");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
