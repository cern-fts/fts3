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
#include <unistd.h>
#include <memory>
#include <algorithm>

#include "cred/TempFile.h"
#include "cred/DelegCred.h"
#include "common/Exceptions.h"

using fts3::common::SystemError;

TEST(CredTest, EmptyPrefix) {
    EXPECT_THROW(TempFile("", "/tmp/"), SystemError);
}

TEST(CretTest, Destructor) {
    auto tempFile = std::make_shared<TempFile>("x509_cred_test", "/tmp");
    std::string filename = tempFile->name();
    EXPECT_EQ(access(tempFile->name().c_str(), F_OK), 0);

    tempFile.reset();
    EXPECT_EQ(access(filename.c_str(), F_OK), -1);
}

TEST(CredTest, Rename) {
    TempFile tempFile("x509_cred_test", "/tmp");
    std::string filename = tempFile.name();
    EXPECT_EQ(access(filename.c_str(), F_OK), 0);

    std::string renamed = "/tmp/x509_cred_test.renamed";
    tempFile.rename(renamed);

    EXPECT_TRUE(tempFile.name().empty());
    EXPECT_EQ(access(filename.c_str(), F_OK), -1);
    EXPECT_EQ(access(renamed.c_str(), F_OK), 0);
    EXPECT_EQ(unlink(renamed.c_str()), 0);
}

TEST(CredTest, ProxyName) {
    const std::string dn = "/DC=ch/DC=cern/OU=Organic Units/OU=Users/CN=ftssuite/CN=123456/CN=Test Certificate";
    const std::string credId = "2a0a5eb0599b90bd";
    const auto hash = std::hash<std::string>()(dn + credId);

    std::stringstream proxyname;
    proxyname << "/tmp/x509up_h" << hash << "_" << credId;
    EXPECT_EQ(DelegCred::generateProxyName(dn, credId), proxyname.str());

    std::string encodedDN = dn;
    std::transform(dn.begin(), dn.end(), encodedDN.begin(),
                   [](unsigned char c) -> unsigned char {
                       return isalnum(c) ? static_cast<unsigned char>(tolower(c)) : 'X';
                   });
    std::stringstream proxyname_legacy;
    proxyname_legacy << "/tmp/x509up_h" << hash << encodedDN;
    EXPECT_EQ(DelegCred::generateProxyName(dn, credId, true), proxyname_legacy.str());
}

TEST(CredTest, InvalidProxy) {
    TempFile tempFile("x509_cred_test", "/tmp");
    std::string message;

    EXPECT_EQ(DelegCred::isValidProxy(tempFile.name(), message), false);
}
