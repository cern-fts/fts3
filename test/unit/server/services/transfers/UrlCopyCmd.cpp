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

#include "server/services/transfers/UrlCopyCmd.h"

using namespace fts3::server;

TEST(TestUrlCopyCmd, TestIpFlags) {
    UrlCopyCmd cmd;

    // Default
    std::string params = cmd.generateParameters();
    EXPECT_EQ(params.find("ipv4"), std::string::npos);
    EXPECT_EQ(params.find("ipv6"), std::string::npos);

    // Explicitly set ipv4
    cmd.setIPv6(true);
    params = cmd.generateParameters();
    EXPECT_EQ(params.find("ipv4"), std::string::npos);
    EXPECT_NE(params.find("ipv6"), std::string::npos);

    // Explicitly set ipv6 to false
    cmd.setIPv6(false);
    params = cmd.generateParameters();
    EXPECT_NE(params.find("ipv4"), std::string::npos);
    EXPECT_EQ(params.find("ipv6"), std::string::npos);
}