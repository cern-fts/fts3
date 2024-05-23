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

#include "common/Exceptions.h"
#include "config/ServerConfig.h"

bool checkException(const fts3::common::UserError &) {
    return true;
}

class ServerConfigFixture : public fts3::config::ServerConfig, public testing::Test {
};

TEST_F(ServerConfigFixture, GetStr) {

    const std::string f_key = "key";
    const std::string f_val = "value";
    _vars[f_key] =
            f_val;

    std::string val = get<std::string>(f_key);
    EXPECT_EQ(val, f_val);
    EXPECT_THROW(val = get<std::string>("notkey"), fts3::common::UserError);
}

struct MockReader {
    typedef std::map<std::string, std::string> type_vars;

    type_vars operator()(int, char **) {
        type_vars ret;
        ret["key"] = "val";
        return ret;
    }
};

TEST_F(ServerConfigFixture, Read) {
    _read<MockReader>(0, NULL);
    EXPECT_EQ(get<std::string>("key"), "val");
}

TEST_F(ServerConfigFixture, getInt) {
    _vars["key"] = "10";
    EXPECT_EQ(get<int>("key"), 10);
}

TEST_F(ServerConfigFixture, getDouble) {
    _vars["key"] = "10.05";
    EXPECT_EQ(get<double>("key"), 10.05);
}
