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
 * TransferStatusCliTest.cpp
 *
 *  Created on: Mar 21, 2012
 *      Author: Michał Simon
 */


//#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/TransferStatusCli.h"
#include "unittest/testsuite.h"

using namespace fts3::cli;

BOOST_AUTO_TEST_CASE (TransferStatusCli_Test)
{

    // has to be const otherwise is deprecated
    char* av[] =
    {
        "prog_name",
        "-l"
    };

    int ac = 2;

    auto_ptr<TransferStatusCli> cli (
        getCli<TransferStatusCli>(ac, av)
    );

    BOOST_CHECK(cli->list());
}

