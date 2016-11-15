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

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "cli/ui/SetCfgCli.h"

using fts3::cli::bad_option;
using fts3::cli::SetCfgCli;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(SetCfgCliTest)


BOOST_AUTO_TEST_CASE (SetCfgCliOverflow)
{
    SetCfgCli cli;
    std::vector<const char*> argv{
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--bring-online",
        "srm://se",
        "11111111111111111111111111111111111111111111111111111111111111111111111111"
    };
    BOOST_CHECK_THROW(cli.parse(argv.size(), (char**)argv.data()), bad_option);
}


BOOST_AUTO_TEST_CASE (SetCfgCliBadLexicalCast)
{
    SetCfgCli cli;
    std::vector<const char*> argv2 {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--bring-online",
        "srm://se",
        "dadadad"
    };
    BOOST_CHECK_THROW(cli.parse(argv2.size(), (char**)argv2.data()), bad_option);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
