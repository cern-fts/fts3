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

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/included/unit_test.hpp>

#include "cli/ui/DnCli.h"

using fts3::cli::DnCli;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(DnCliTest)


BOOST_AUTO_TEST_CASE (DnCliShortOptions)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-u",
        "userdn"
    };

    DnCli cli;
    cli.parse(argv.size(), (char**)argv.data());

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(!cli.getUserDn().empty());
    // the endpoint shouldn't be empty since it's starting with http
    BOOST_CHECK(cli.getUserDn().compare("userdn") == 0);
}


BOOST_AUTO_TEST_CASE (DnCliLongOptions)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--userdn",
        "userdn"
    };

    DnCli cli;
    cli.parse(argv.size(), (char**)argv.data());

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(!cli.getUserDn().empty());
    // the endpoint shouldn't be empty since it's starting with http
    BOOST_CHECK(cli.getUserDn().compare("userdn") == 0);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
