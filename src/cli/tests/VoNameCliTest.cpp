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
 * ListVOManagerCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michal Simon
 */


#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "ui/VoNameCli.h"
#include "unittest/testsuite.h"

#include <iostream>
#include <memory>

using namespace fts3::cli;
using namespace std;

BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(VONameCliTest)

BOOST_AUTO_TEST_CASE (VONameCli_options)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "voname"
    };

    int ac = 2;

    unique_ptr<VoNameCli> cli(new VoNameCli);
    cli->parse(ac, av);

    BOOST_CHECK(cli->getVoName() == "voname");
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS
