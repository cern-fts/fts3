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
 * SrvManagerTest.cpp
 *
 *  Created on: Feb 24, 2012
 *      Author: Michal Simon
 */
//#ifdef FTS3_COMPILE_WITH_UNITTEST

#include "gsoap_transfer_proxy.h"
#include "SrvManager.h"

#include "unittest/testsuite.h"
#include "server/ws/WebServerMock.h"
#include "common/InstanceHolder.h"

using namespace fts3::cli;
using namespace fts3::ws;
using namespace fts3::common;
using namespace std;

typedef InstanceHolder<FileTransferSoapBindingProxy> ServiceProxyInstanceHolder;


class SrvManagerTester: public SrvManager {

public :
	SrvManagerTester(){};
};

BOOST_FIXTURE_TEST_CASE (SrvManager_init, SrvManagerTester) {

	// create and start mock-server
	WebServerMock* ws = WebServerMock::getInstance();
	ws->run(8899);

	// wait a while until the mock-server is running
	sleep(2);

	// create and initialize the service-proxy
	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	service.soap_endpoint = "http://localhost:8899";

	// initialize SrvManager
	init(service);

	BOOST_CHECK(getVersion().compare(WebServerMock::VERSION) == 0);
	BOOST_CHECK(getInterface().compare(WebServerMock::INTERFACE) == 0);
	BOOST_CHECK(getSchema().compare(WebServerMock::SCHEMA) == 0);
	BOOST_CHECK(getMetadata().compare(WebServerMock::METADATA) == 0);
}


BOOST_FIXTURE_TEST_CASE (SrvManager_setInterfaceVersion, SrvManagerTester) {

	setInterfaceVersion("0.0.1");
	BOOST_CHECK(patch == 1 && minor == 0 && major == 0);

	setInterfaceVersion("1.2.");
	BOOST_CHECK(minor == 2 && major == 1);

	setInterfaceVersion("3.");
	BOOST_CHECK(major == 3);
}


//#endif // FTS3_COMPILE_WITH_UNITTESTS
