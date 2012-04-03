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

#include "gsoap_transfer_proxy.h"

#include "ui/SubmitTransferCli.h"
#include "unittest/testsuite.h"

#include "common/JobParameterHandler.h"
#include "common/InstanceHolder.h"

#include <fstream>
#include <iostream>

using namespace std;

using namespace fts3::cli;
using namespace fts3::common;


typedef InstanceHolder<FileTransferSoapBindingProxy> ServiceProxyInstanceHolder;


BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Test_File, SubmitTransferCli) {

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

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name",
			"-f", "/tmp/bulk"
	};

	initCli(3, const_cast<char**>(av));

	BOOST_CHECK(createJobElements());
	BOOST_CHECK(useCheckSum());

	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	vector<tns3__TransferJobElement2*> elements = getJobElements2(&service);

	BOOST_CHECK(elements.size() == 2);

	BOOST_CHECK(elements[0]->source->compare("source1") == 0);
	BOOST_CHECK(elements[0]->dest->compare("destination1") == 0);
	BOOST_CHECK(elements[0]->checksum->compare("Alg:check1") == 0);

	BOOST_CHECK(elements[1]->source->compare("source2") == 0);
	BOOST_CHECK(elements[1]->dest->compare("destination2") == 0);
	BOOST_CHECK(elements[1]->checksum->empty());
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Test_OtherOptions, SubmitTransferCli) {
	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name",
			"-b",
			"-p", "P@ssw0rd",
			"-i", "23",
			"-e", "1234"
	};

	initCli(8, const_cast<char**>(av));

	BOOST_CHECK(isBlocking());
	BOOST_CHECK(vm["interval"].as<int>() == 23);
	BOOST_CHECK(vm["expire"].as<long>() == 1234);

	mute();
	performChecks();
	unmute();

	BOOST_CHECK(getPassword().compare("P@ssw0rd") == 0);
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Test_JobElements, SubmitTransferCli) {

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name",
			"source",
			"destination"
	};

	initCli(3, const_cast<char**>(av));

	BOOST_CHECK(getSource().compare("source") == 0);
	BOOST_CHECK(getDestination().compare("destination") == 0);

	BOOST_CHECK(createJobElements());
	BOOST_CHECK(!useCheckSum());

	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	vector<tns3__TransferJobElement*> elements = getJobElements(&service);

	BOOST_CHECK(elements.size() == 1);
	BOOST_CHECK(elements[0]->source->compare("source") == 0);
	BOOST_CHECK(elements[0]->dest->compare("destination") == 0);
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Test_JobElements2, SubmitTransferCli) {

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name",
			"source",
			"destination",
			"ALGORITHM:1234af"
	};

	initCli(4, const_cast<char**>(av));

	BOOST_CHECK(getSource().compare("source") == 0);
	BOOST_CHECK(getDestination().compare("destination") == 0);

	BOOST_CHECK(createJobElements());
	BOOST_CHECK(useCheckSum());

	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	vector<tns3__TransferJobElement2*> elements = getJobElements2(&service);

	BOOST_CHECK(elements.size() == 1);
	BOOST_CHECK(elements[0]->source->compare("source") == 0);
	BOOST_CHECK(elements[0]->dest->compare("destination") == 0);
	BOOST_CHECK(elements[0]->checksum->compare("ALGORITHM:1234af") == 0);
}

BOOST_FIXTURE_TEST_CASE (SubmitTransferCli_Parameters_Test, SubmitTransferCli) {

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name",
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

	initCli(17, const_cast<char**>(av));

	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	tns3__TransferParams* params = getParams(&service);

	JobParameterHandler handler;
	handler(params->keys, params->values);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD).compare("compare") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG).compare("Y") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_LAN_CONNECTION).compare("Y") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE).compare("Y") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_GRIDFTP).compare("gparam") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_MYPROXY).compare("myproxysrv") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_DELEGATIONID).compare("id") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_SPACETOKEN).compare("dest-token") == 0);
	BOOST_CHECK(handler.get(JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE).compare("source-token") == 0);
	BOOST_CHECK(handler.get<int>(JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME) == 123);
}

