/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ListTransferCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michal Simon
 */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "ui/ListTransferCli.h"
#include "unittest/testsuite.h"

using namespace fts3::cli;


BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(ListTransferCliTest)

BOOST_AUTO_TEST_CASE (ListTransferCli_vo_short_option)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-o",
        "vo"
    };

    // argument count
    int ac = 5;

    std::unique_ptr<ListTransferCli> cli (new ListTransferCli);
    cli->parse(ac, av);
    BOOST_CHECK_EQUAL(cli->getVoName(), "vo");
}

BOOST_AUTO_TEST_CASE (ListTransferCli_vo_long_option)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--voname",
        "vo"
    };

    // argument count
    int ac = 5;

    std::unique_ptr<ListTransferCli> cli (new ListTransferCli);
    cli->parse(ac, av);

    BOOST_CHECK(cli->getVoName() == "vo");
}

BOOST_AUTO_TEST_CASE (ListTransferCli_Status)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
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

    // argument count
    int ac = 9;

    std::unique_ptr<ListTransferCli> cli (new ListTransferCli);
    cli->parse(ac, av);
    cli->validate();

    const std::vector<std::string>& statuses = cli->getStatusArray();
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

#endif // FTS3_COMPILE_WITH_UNITTESTS
