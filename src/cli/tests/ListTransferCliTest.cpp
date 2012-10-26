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

//#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/ListTransferCli.h"
#include "unittest/testsuite.h"

using namespace fts3::cli;


BOOST_AUTO_TEST_CASE (ListTransferCli_VO_Test1) {

        // has to be const otherwise is deprecated
        char* av[] = {
        		"prog_name",
    			"-s",
    			"https://fts3-server:8080",
        		"-o",
        		"vo"
        	};

    	// argument count
    	int ac = 5;

    	auto_ptr<ListTransferCli> cli (
    			getCli<ListTransferCli>(ac, av)
    		);

        BOOST_CHECK(cli->getVoName().compare("vo") == 0);
}

BOOST_AUTO_TEST_CASE (ListTransferCli_VO_Test2) {

        // has to be const otherwise is deprecated
        char* av[] = {
        		"prog_name",
    			"-s",
    			"https://fts3-server:8080",
        		"--voname",
        		"vo"
        	};

    	// argument count
    	int ac = 5;

    	auto_ptr<ListTransferCli> cli (
    			getCli<ListTransferCli>(ac, av)
    		);

        BOOST_CHECK(cli->getVoName() == "vo");
}

BOOST_AUTO_TEST_CASE (ListTransferCli_Status_Test) {

	// has to be const otherwise is deprecated
	char* av[] = {
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

	auto_ptr<ListTransferCli> cli (
			getCli<ListTransferCli>(ac, av)
		);

	cli->validate(false);

	const vector<string>& statuses = cli->getStatusArray();
	BOOST_CHECK(statuses.size() == 6);
	BOOST_CHECK(statuses[0] == "status1");
	BOOST_CHECK(statuses[1] == "status2");
	BOOST_CHECK(statuses[2] == "status3");
	BOOST_CHECK(statuses[3] == "status4");
	BOOST_CHECK(statuses[4] == "status5");
	BOOST_CHECK(statuses[5] == "status6");
}

