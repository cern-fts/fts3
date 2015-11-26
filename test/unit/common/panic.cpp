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

#include <signal.h>
#include <boost/test/included/unit_test.hpp>
#include <boost/thread.hpp>

#include "common/panic.h"

// Not in the public header, so need to declare here
namespace fts3 {
    namespace common {
        namespace panic {
            void get_backtrace(int signum);
        }
    }
}

using namespace fts3::common::panic;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(panicTest)


void shutdown_callback(int, void *udata)
{
    int *counter = (int*)udata;
    ++(*counter);
}


BOOST_AUTO_TEST_CASE(basic)
{
    int counter = 0;
    setup_signal_handlers(shutdown_callback, &counter);

    raise(SIGUSR1);
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    BOOST_CHECK_EQUAL(counter, 1);
}


BOOST_AUTO_TEST_CASE(backtraceTest)
{
    /// This will print the backtrace into stderr
    get_backtrace(SIGSEGV);
    std::string trace = stack_dump(stack_backtrace, stack_backtrace_size);

    BOOST_CHECK_NE(trace.find("backtraceTest"), std::string::npos);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
