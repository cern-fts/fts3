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
 * CliBaseTest.cpp
 *
 *  Created on: Feb 23, 2012
 *      Author: Michal Simon
 */


#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "ui/CliBase.h"
#include "unittest/testsuite.h"

#include <memory>
#include <sstream>

using namespace fts3::cli;
using namespace std;

stringstream out; //> use this instead of std::cout

/*
 * Dummy class that inherits after abstract CliBase
 * and implements its pure virtual method so the class
 * can be instantiated and tested
 */
class CliBaseTester : public CliBase
{

public:
    CliBaseTester() : CliBase(out) {}

    // implement the pure vitual method
    string getUsageString(string tool)
    {
        return tool;
    };
};

BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(CliBaseTest)

BOOST_AUTO_TEST_CASE (CliBase_short_options)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-h",
        "-q",
        "-v",
        "-s",
        "https://fts3-server:8080",
        "-V"
    };

    // argument count
    int ac = 7;

    unique_ptr<CliBaseTester> cli (new CliBaseTester);
    cli->parse(ac, av);

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(cli->printHelp(string()));
    BOOST_CHECK(cli->isQuite());
    BOOST_CHECK(cli->isVerbose());
    BOOST_CHECK(cli->printVersion());

    // the endpoint shouldn't be empty since it's starting with http
    BOOST_CHECK(!cli->getService().empty());
}

BOOST_AUTO_TEST_CASE (CliBase_long_options)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "--help",
        "--quite",
        "--verbose",
        "--service",
        "https://fts3-server:8080",
        "--version"
    };

    // argument count
    int ac = 7;

    unique_ptr<CliBaseTester> cli (new CliBaseTester);
    cli->parse(ac, av);

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(cli->printHelp(string()));
    BOOST_CHECK(cli->isQuite());
    BOOST_CHECK(cli->isVerbose());
    BOOST_CHECK(cli->printVersion());
    // the endpoint should be empty since it's not starting with http, https, httpd
    BOOST_CHECK(!cli->getService().empty());
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()


#endif // FTS3_COMPILE_WITH_UNITTESTS
