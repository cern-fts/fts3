/*
 * Copyright (c) CERN 2017
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

#include "server/services/transfers/UrlCopyCmd.h"

using namespace fts3::server;

BOOST_AUTO_TEST_SUITE(server)
BOOST_AUTO_TEST_SUITE(UrlCopyCmdTestSuite)

/**
 * Test IPv4/IPv6 flag combinations
 */
BOOST_AUTO_TEST_CASE (TestIpFlags)
{
    UrlCopyCmd cmd;

    // Default
    std::string params = cmd.generateParameters();
    BOOST_CHECK_EQUAL(params.find("ipv4"), std::string::npos);
    BOOST_CHECK_EQUAL(params.find("ipv6"), std::string::npos);

    // Explicitly set ipv4
    cmd.setIPv6(true);
    params = cmd.generateParameters();
    BOOST_CHECK_EQUAL(params.find("ipv4"), std::string::npos);
    BOOST_CHECK_NE(params.find("ipv6"), std::string::npos);

    // Explicitly set ipv6 to false
    cmd.setIPv6(false);
    params = cmd.generateParameters();
    BOOST_CHECK_NE(params.find("ipv4"), std::string::npos);
    BOOST_CHECK_EQUAL(params.find("ipv6"), std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
