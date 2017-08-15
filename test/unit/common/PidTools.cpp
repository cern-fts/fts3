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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>
#include "common/PidTools.h"

using namespace fts3::common;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(PidTools)


BOOST_AUTO_TEST_CASE(basic)
{
    uint64_t selfStartTime = getPidStartime(getpid());

    BOOST_CHECK_GT(selfStartTime, (time(NULL) - 120) * 1000);
    BOOST_CHECK_LT(selfStartTime, (time(NULL) + 1) * 1000);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
