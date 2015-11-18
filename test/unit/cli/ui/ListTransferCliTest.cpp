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

#include "cli/ui/ListTransferCli.h"

using fts3::cli::ListTransferCli;


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(ListTransferCliTest)


BOOST_AUTO_TEST_CASE (ListTransferCliVoShortOption)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-o",
        "vo"
    };

    ListTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    BOOST_CHECK_EQUAL(cli.getVoName(), "vo");
}


BOOST_AUTO_TEST_CASE (ListTransferCliVoLongOption)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--voname",
        "vo"
    };

    ListTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());

    BOOST_CHECK(cli.getVoName() == "vo");
}


BOOST_AUTO_TEST_CASE (ListTransferCli_Status)
{
    std::vector<const char*> argv {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "status1",
        "status2",
        "status3",
        "status4",
        "status5",
        "status6"
    };

    ListTransferCli cli;
    cli.parse(argv.size(), (char**)argv.data());
    cli.validate();

    const std::vector<std::string>& statuses = cli.getStatusArray();
    BOOST_CHECK(statuses.size() == 6);
    BOOST_CHECK(statuses[0] == "status1");
    BOOST_CHECK(statuses[1] == "status2");
    BOOST_CHECK(statuses[2] == "status3");
    BOOST_CHECK(statuses[3] == "status4");
    BOOST_CHECK(statuses[4] == "status5");
    BOOST_CHECK(statuses[5] == "status6");
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
