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

#include "common/UserProxyEnv.h"


BOOST_AUTO_TEST_SUITE(common)
BOOST_AUTO_TEST_SUITE(UserProxyEnvTest)


BOOST_AUTO_TEST_CASE (recoverPreviousNull)
{
    unsetenv("X509_USER_KEY");
    unsetenv("X509_USER_CERT");
    unsetenv("X509_USER_PROXY");

    char *key = getenv("X509_USER_KEY");
    char *cert = getenv("X509_USER_CERT");
    char *proxy = getenv("X509_USER_PROXY");

    BOOST_CHECK_EQUAL(key, (void*)NULL);
    BOOST_CHECK_EQUAL(cert, (void*)NULL);
    BOOST_CHECK_EQUAL(proxy, (void*)NULL);

    {
        UserProxyEnv uproxy("TESTPROXY");

        key = getenv("X509_USER_KEY");
        cert = getenv("X509_USER_CERT");
        proxy = getenv("X509_USER_PROXY");

        BOOST_CHECK_EQUAL(std::string(key), "TESTPROXY");
        BOOST_CHECK_EQUAL(std::string(cert), "TESTPROXY");
        BOOST_CHECK_EQUAL(std::string(proxy), "TESTPROXY");
    }

    key = getenv("X509_USER_KEY");
    cert = getenv("X509_USER_CERT");
    proxy = getenv("X509_USER_PROXY");

    BOOST_CHECK_EQUAL(key, (void*)NULL);
    BOOST_CHECK_EQUAL(cert, (void*)NULL);
    BOOST_CHECK_EQUAL(proxy, (void*)NULL);
}


BOOST_AUTO_TEST_CASE (recoverPrevious)
{
    setenv("X509_USER_KEY", "aaaa", 1);
    setenv("X509_USER_CERT", "bbbb", 1);
    setenv("X509_USER_PROXY", "cccc", 1);

    char *key = getenv("X509_USER_KEY");
    char *cert = getenv("X509_USER_CERT");
    char *proxy = getenv("X509_USER_PROXY");

    {
        UserProxyEnv uproxy("TESTPROXY");

        key = getenv("X509_USER_KEY");
        cert = getenv("X509_USER_CERT");
        proxy = getenv("X509_USER_PROXY");

        BOOST_CHECK_EQUAL(std::string(key), "TESTPROXY");
        BOOST_CHECK_EQUAL(std::string(cert), "TESTPROXY");
        BOOST_CHECK_EQUAL(std::string(proxy), "TESTPROXY");
    }

    key = getenv("X509_USER_KEY");
    cert = getenv("X509_USER_CERT");
    proxy = getenv("X509_USER_PROXY");

    BOOST_CHECK_EQUAL(std::string(key), "aaaa");
    BOOST_CHECK_EQUAL(std::string(cert), "bbbb");
    BOOST_CHECK_EQUAL(std::string(proxy), "cccc");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
