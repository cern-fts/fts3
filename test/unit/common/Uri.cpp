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

#include "common/Uri.h"

using namespace fts3::common;

TEST(UriTest, Basic) {
    Uri uri = Uri::parse("http://subdomain.domain.com:8080/path?query=args");

    EXPECT_EQ(uri.protocol, "http");
    EXPECT_EQ(uri.host, "subdomain.domain.com");
    EXPECT_EQ(uri.port, 8080);
    EXPECT_EQ(uri.path, "/path");
    EXPECT_EQ(uri.queryString, "query=args");
    EXPECT_EQ(uri.getSeName(), "http://subdomain.domain.com");
}

TEST(UriTest, Http3rd) {
    Uri uri = Uri::parse("davs+3rd://subdomain.domain.com:8080/path?query=args");
    EXPECT_EQ(uri.protocol, "davs+3rd");
    EXPECT_EQ(uri.getSeName(), "davs+3rd://subdomain.domain.com");
}

TEST(UriTest, SimpleHost) {
    Uri uri = Uri::parse("gsiftp://hostname:2121/path?query=args");
    EXPECT_EQ(uri.protocol, "gsiftp");
    EXPECT_EQ(uri.host, "hostname");
    EXPECT_EQ(uri.port, 2121);
}

TEST(UriTest, LanTransfer) {
    EXPECT_EQ(isLanTransfer("subdomain.domain.com", "subdomain.domain.com"), true);
    EXPECT_EQ(isLanTransfer("subdomain.domain.com", "subdomain2.domain.com"), true);
    EXPECT_EQ(isLanTransfer("subdomain.domain.com", "subdomain.somewhere.com"), false);
    EXPECT_EQ(isLanTransfer("subdomain.domain.com", "subdomain.domain.es"), false);
}
