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


#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/VoNameCli.h"
#include "unittest/testsuite.h"

#include <iostream>

using namespace fts3::cli;
using namespace std;

BOOST_AUTO_TEST_CASE (VONameCli_Test)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "voname"
    };

    int ac = 2;

    auto_ptr<VoNameCli> cli (
        getCli<VoNameCli>(ac, av)
    );

    BOOST_CHECK(cli->getVoName() == "voname");
}
#endif // FTS3_COMPILE_WITH_UNITTESTS
