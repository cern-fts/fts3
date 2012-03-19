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
#include "ServiceProxyHolder.h"
#include "common/JobParameterHandler.h"

using namespace fts3::cli;
using namespace fts3::common;


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

	FileTransferSoapBindingProxy& service = ServiceProxyHolder::getServiceProxy();
	transfer__TransferParams* params = getParams(&service);

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

