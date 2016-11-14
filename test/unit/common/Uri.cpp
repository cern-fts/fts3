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

#include "common/Uri.h"

using namespace fts3::common;


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(UriTest)


BOOST_AUTO_TEST_CASE(basic)
{
    Uri uri = Uri::parse("http://subdomain.domain.com:8080/path?query=args");

    BOOST_CHECK_EQUAL(uri.protocol, "http");
    BOOST_CHECK_EQUAL(uri.host, "subdomain.domain.com");
    BOOST_CHECK_EQUAL(uri.port, 8080);
    BOOST_CHECK_EQUAL(uri.path, "/path");
    BOOST_CHECK_EQUAL(uri.queryString, "query=args");
    BOOST_CHECK_EQUAL(uri.getSeName(), "http://subdomain.domain.com");
}


BOOST_AUTO_TEST_CASE(http3rd)
{
    Uri uri = Uri::parse("davs+3rd://subdomain.domain.com:8080/path?query=args");
    BOOST_CHECK_EQUAL(uri.protocol, "davs+3rd");
    BOOST_CHECK_EQUAL(uri.getSeName(), "davs+3rd://subdomain.domain.com");
}


BOOST_AUTO_TEST_CASE(simpleHost)
{
    Uri uri = Uri::parse("gsiftp://hostname:2121/path?query=args");
    BOOST_CHECK_EQUAL(uri.protocol, "gsiftp");
    BOOST_CHECK_EQUAL(uri.host, "hostname");
    BOOST_CHECK_EQUAL(uri.port, 2121);
}


BOOST_AUTO_TEST_CASE(lanTransfer)
{
    BOOST_CHECK_EQUAL(isLanTransfer("subdomain.domain.com", "subdomain.domain.com"), true);
    BOOST_CHECK_EQUAL(isLanTransfer("subdomain.domain.com", "subdomain2.domain.com"), true);
    BOOST_CHECK_EQUAL(isLanTransfer("subdomain.domain.com", "subdomain.somewhere.com"), false);
    BOOST_CHECK_EQUAL(isLanTransfer("subdomain.domain.com", "subdomain.domain.es"), false);
}


BOOST_AUTO_TEST_CASE(hostname)
{
    char hostname[512];
    FILE *hostnameFile = popen("/bin/hostname -f", "r");
    fgets(hostname, sizeof(hostname), hostnameFile);
    pclose(hostnameFile);
    hostname[strlen(hostname) - 1] = '\0';

    BOOST_CHECK_EQUAL(getFullHostname(), hostname);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
