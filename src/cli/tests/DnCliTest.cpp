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
 * DnCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michał Simon
 */


#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "ui/DnCli.h"
#include "unittest/testsuite.h"

#include <memory>

using namespace fts3::cli;

BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(DnCliTest)

BOOST_AUTO_TEST_CASE (DnCli_short_options)
{
    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "-u",
        "userdn"
    };

    // argument count
    int ac = 5;

    std::unique_ptr<DnCli> cli (new DnCli);
    cli->parse(ac, av);

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(!cli->getUserDn().empty());
    // the endpoint shouldn't be empty since it's starting with http
    BOOST_CHECK(cli->getUserDn().compare("userdn") == 0);
}

BOOST_AUTO_TEST_CASE (DnCli_long_options)
{
    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--userdn",
        "userdn"
    };

    int ac = 5;

    std::unique_ptr<DnCli> cli (new DnCli);
    cli->parse(ac, av);

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(!cli->getUserDn().empty());
    // the endpoint shouldn't be empty since it's starting with http
    BOOST_CHECK(cli->getUserDn().compare("userdn") == 0);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS
