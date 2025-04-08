/*
 * Copyright (c) CERN 2022
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
#include <unistd.h>

#include "cred/TempFile.h"
#include "cred/DelegCred.h"
#include "common/Exceptions.h"

using fts3::common::SystemError;

BOOST_AUTO_TEST_SUITE(cred)
BOOST_AUTO_TEST_SUITE(TempFileTestSuite)

BOOST_AUTO_TEST_CASE(EmptyPrefix)
{
    BOOST_CHECK_THROW(TempFile("", "/tmp/"), SystemError);
}

BOOST_AUTO_TEST_CASE(Destructor)
{
    auto tempFile = std::make_shared<TempFile>("x509_cred_test", "/tmp");
    std::string filename = tempFile->name();
    BOOST_CHECK_EQUAL(access(tempFile->name().c_str(), F_OK), 0);

    tempFile.reset();
    BOOST_CHECK_EQUAL(access(filename.c_str(), F_OK), -1);
}

BOOST_AUTO_TEST_CASE(Rename)
{
    TempFile tempFile("x509_cred_test", "/tmp");
    std::string filename = tempFile.name();
    BOOST_CHECK_EQUAL(access(filename.c_str(), F_OK), 0);

    std::string renamed = "/tmp/x509_cred_test.renamed";
    tempFile.rename(renamed);

    BOOST_CHECK(tempFile.name().empty());
    BOOST_CHECK_EQUAL(access(filename.c_str(), F_OK), -1);
    BOOST_CHECK_EQUAL(access(renamed.c_str(), F_OK), 0);
    BOOST_CHECK_EQUAL(unlink(renamed.c_str()), 0);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE(DelegCredTestSuite)

BOOST_AUTO_TEST_CASE(ProxyName)
{
    const std::string dn = "/DC=ch/DC=cern/OU=Organic Units/OU=Users/CN=ftssuite/CN=123456/CN=Test Certificate";
    const std::string credId = "2a0a5eb0599b90bd";
    const auto hash = std::hash<std::string>()(dn + credId);

    std::stringstream proxyname;
    proxyname << "/tmp/x509up_h" << hash << "_" << credId;
    BOOST_CHECK_EQUAL(DelegCred::generateProxyName(dn, credId), proxyname.str());
}

BOOST_AUTO_TEST_CASE(InvalidProxy)
{
    TempFile tempFile("x509_cred_test", "/tmp");
    std::string message;

    BOOST_CHECK_EQUAL(DelegCred::isValidProxy(tempFile.name(), message), false);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
