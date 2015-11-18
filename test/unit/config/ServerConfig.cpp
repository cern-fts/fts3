/*
 * Copyright (c) CERN 2013-2015
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

#include <boost/test/included/unit_test.hpp>

#include "common/Exceptions.h"
#include "config/ServerConfig.h"


BOOST_AUTO_TEST_SUITE(config)
BOOST_AUTO_TEST_SUITE(ServerConfigTestSuite)


bool checkException(const fts3::common::UserError&)
{
    return true;
}


BOOST_FIXTURE_TEST_CASE(getStr, fts3::config::ServerConfig)
{
    const std::string f_key = "key";
    const std::string f_val = "value";
    _vars[f_key] =
    f_val;

    std::string val = get<std::string>(f_key);
    BOOST_CHECK_EQUAL (val, f_val);
    BOOST_CHECK_THROW(val = get<std::string>("notkey"), fts3::common::UserError);
}


struct MockReader {
    typedef std::map<std::string, std::string> type_vars;

    type_vars operator()(int, char **)
    {
        type_vars ret;
        ret["key"] = "val";
        return ret;
    }
};


BOOST_FIXTURE_TEST_CASE (read, fts3::config::ServerConfig)
{
    _read<MockReader> (0, NULL);
    BOOST_CHECK_EQUAL (get<std::string>("key"), "val");
}


BOOST_FIXTURE_TEST_CASE (getInt, fts3::config::ServerConfig)
{
    _vars["key"] = "10";
    BOOST_CHECK_EQUAL (get<int>("key"), 10);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
