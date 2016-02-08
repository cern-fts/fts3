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

#include <boost/test/included/unit_test.hpp>
#include "url-copy/AutoInterruptThread.h"


BOOST_AUTO_TEST_SUITE(url_copy)

void func(bool *flag)
{
    try {
        while (!boost::this_thread::interruption_requested()) {
            boost::this_thread::sleep(boost::posix_time::millisec(100));
        }
        *flag = true;
    }
    catch (const boost::thread_interrupted&) {
        *flag = true;
    }
    catch (...) {
        // pass
    }
}


BOOST_AUTO_TEST_CASE(simple)
{
    bool interruptCalled = false;
    AutoInterruptThread *thread = new AutoInterruptThread(boost::bind(func, &interruptCalled));
    boost::this_thread::sleep(boost::posix_time::millisec(500));
    delete thread;
    boost::this_thread::sleep(boost::posix_time::millisec(500));
    BOOST_CHECK(interruptCalled);
}


BOOST_AUTO_TEST_SUITE_END()
