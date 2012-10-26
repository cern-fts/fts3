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
 * SubmitTransferCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michal Simon
 */

//#ifdef FTS3_COMPILE_WITH_UNITTEST

#include "ui/SubmitTransferCli.h"
#include "unittest/testsuite.h"

#include "common/JobParameterHandler.h"

#include <fstream>
#include <iostream>
#include <memory>

using namespace std;

using namespace fts3::cli;
using namespace fts3::common;


BOOST_AUTO_TEST_CASE (SubmitTransferCli_Test_File) {

	// creat a tmeporary file with bulk-job description
	fstream fs("/tmp/bulk", fstream::out);
	if (!fs) {
		cout << "It was not possible to carry out the bulk-job test!" << endl;
		return;
	}

	fs << "source1 destination1 Alg:check1" << endl;
	fs << "source2 destination2" << endl;

	fs.flush();
	fs.close();

	// argument vector
	char* av[] = {
			"prog_name",
			"-s",
			"https://fts3-server:8080",
			"-f",
			"/tmp/bulk"
		};

	// argument count
	int ac = 5;

	auto_ptr<SubmitTransferCli> cli (
			getCli<SubmitTransferCli>(ac, av)
		);

	cli->mute();
	cli->validate(false);

	BOOST_CHECK(cli->useCheckSum());

	vector<JobElement> elements = cli->getJobElements();

	BOOST_CHECK(elements.size() == 2);

	BOOST_CHECK(elements[0].get<0>().compare("source1") == 0);
	BOOST_CHECK(elements[0].get<1>().compare("destination1") == 0);
	BOOST_CHECK(elements[0].get<2>().get().compare("Alg:check1") == 0);

	BOOST_CHECK(elements[1].get<0>().compare("source2") == 0);
	BOOST_CHECK(elements[1].get<1>().compare("destination2") == 0);
	BOOST_CHECK(elements[1].get<2>().is_initialized() == false);

	cli->unmute();
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_Test_OtherOptions) {
	// has to be const otherwise is deprecated
	char* av[] = {
			"prog_name",
			"-s",
			"https://fts3-server:8080",
			"-b",
			"-p",
			"P@ssw0rd",
			"-i",
			"23",
			"-e",
			"1234"
		};

	// argument count
	int ac = 10;

	auto_ptr<SubmitTransferCli> cli (
			getCli<SubmitTransferCli>(ac, av)
		);

	cli->mute();
	cli->validate(false);

	BOOST_CHECK(cli->isBlocking());
//	BOOST_CHECK(vm["interval"].as<int>() == 23);
	BOOST_CHECK(cli->getExpirationTime() == 1234);

	BOOST_CHECK(cli->getPassword().compare("P@ssw0rd") == 0);

	cli->unmute();
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_Test_JobElements) {

	// has to be const otherwise is deprecated
	char* av[] = {
			"prog_name",
			"-s",
			"https://fts3-server:8080",
			"source",
			"destination"
		};

	// argument count
	int ac = 5;

	auto_ptr<SubmitTransferCli> cli (
			getCli<SubmitTransferCli>(ac, av)
		);

	cli->mute();
	cli->validate(false);

	BOOST_CHECK(cli->getSource().compare("source") == 0);
	BOOST_CHECK(cli->getDestination().compare("destination") == 0);
	BOOST_CHECK(!cli->useCheckSum());

	vector<JobElement> elements = cli->getJobElements();

	BOOST_CHECK(elements.size() == 1);
	BOOST_CHECK(elements[0].get<0>().compare("source") == 0);
	BOOST_CHECK(elements[0].get<1>().compare("destination") == 0);

	cli->unmute();
}

BOOST_AUTO_TEST_CASE (SubmitTransferCli_Test_JobElements2) {

	// has to be const otherwise is deprecated
	char* av[] = {
			"prog_name",
			"-s",
			"https://fts3-server:8080",
			"source",
			"destination",
			"ALGORITHM:1234af"
		};
	// argument count
	int ac = 6;

	auto_ptr<SubmitTransferCli> cli (
			getCli<SubmitTransferCli>(ac, av)
		);

	cli->mute();
	cli->validate(false);

	BOOST_CHECK(cli->getSource().compare("source") == 0);
	BOOST_CHECK(cli->getDestination().compare("destination") == 0);
	BOOST_CHECK(cli->useCheckSum());

	vector<JobElement> elements = cli->getJobElements();

	BOOST_CHECK(elements.size() == 1);
	BOOST_CHECK(elements[0].get<0>().compare("source") == 0);
	BOOST_CHECK(elements[0].get<1>().compare("destination") == 0);
	BOOST_CHECK(elements[0].get<2>().get().compare("ALGORITHM:1234af") == 0);

	cli->unmute();
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Parameters_Test, SubmitTransferCli) {

	// has to be const otherwise is deprecated
	char* av[] = {
			"prog_name",
			"-s",
			"https://fts3-server:8080",
			"-o",
			"-K",
			"--lan-connection",
			"--fail-nearline",
			"-g", "gparam",
			"-m", "myproxysrv",
			"-I", "id",
			"-t", "dest-token",
			"-S", "source-token",
			"--copy-pin-lifetime", "123"
		};
	// argument count
	int ac = 19;

	auto_ptr<SubmitTransferCli> cli (
			getCli<SubmitTransferCli>(ac, av)
		);

	cli->mute();
	cli->validate(false);

	map<string, string> params = cli->getParams();

	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD).compare("compare") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG).compare("Y") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_LAN_CONNECTION).compare("Y") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE).compare("Y") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_GRIDFTP).compare("gparam") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_MYPROXY).compare("myproxysrv") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_DELEGATIONID).compare("id") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_SPACETOKEN).compare("dest-token") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE).compare("source-token") == 0);
	BOOST_CHECK(params.at(JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME).compare("123"));

	cli->unmute();
}

