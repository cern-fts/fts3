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
 * JobIDCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michal Simon
 */


//#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/JobIdCli.h"
#include "unittest/testsuite.h"

#include <string>
#include <vector>

using namespace std;
using namespace fts3::cli;

BOOST_AUTO_TEST_CASE (JobIDCli_Test1)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "ID1",
        "ID2",
        "ID3"
    };

    int ac = 6;

    auto_ptr<JobIdCli> cli (
        getCli<JobIdCli>(ac, av)
    );

    cli->validate(false);

    const vector<string>& ids = cli->getJobIds();

    // the vector should have 3 elements
    BOOST_CHECK(ids.size() == 3);
    // check if the values are correct
    BOOST_CHECK(ids[0] == "ID1");
    BOOST_CHECK(ids[1] == "ID2");
    BOOST_CHECK(ids[2] == "ID3");
}
