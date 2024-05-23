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
#include <signal.h>
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

void shutdown_callback(int, void *udata) {
    int *counter = (int *) udata;
    ++(*counter);
}


TEST(PanicTest, Basic) {
    int counter = 0;
    setup_signal_handlers(shutdown_callback, &counter);

    raise(SIGUSR1);
    boost::this_thread::sleep(boost::posix_time::seconds(1));

    EXPECT_EQ(counter, 1);
}


TEST(PanicTest, BacktraceTest) {
    /// This will print the backtrace into stderr
    get_backtrace(SIGSEGV);
    std::string trace = stack_dump(stack_backtrace, stack_backtrace_size);

    EXPECT_NE(trace.find("BacktraceTest"), std::string::npos);
}
