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
#include "gsoap_transfer_proxy.h"

#include "ui/ListTransferCli.h"
#include "unittest/testsuite.h"
#include "common/InstanceHolder.h"

using namespace fts3::cli;
using namespace fts3::common;


typedef InstanceHolder<FileTransferSoapBindingProxy> ServiceProxyInstanceHolder;

BOOST_FIXTURE_TEST_CASE (ListTransferCli_VO_Test1, ListTransferCli) {

        // has to be const otherwise is deprecated
        const char* av[] = {"prog_name", "-o", "vo"};
        initCli(4, const_cast<char**>(av));

        BOOST_CHECK(getVOName().compare("vo") == 0);
}

BOOST_FIXTURE_TEST_CASE (ListTransferCli_VO_Test2, ListTransferCli) {

        // has to be const otherwise is deprecated
        const char* av[] = {"prog_name", "--voname", "vo"};
        initCli(4, const_cast<char**>(av));

        BOOST_CHECK(getVOName().compare("vo") == 0);
}

BOOST_FIXTURE_TEST_CASE (ListTransferCli_Status_Test, ListTransferCli) {

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name", "status1", "status2", "status3", "status4", "status5", "status6"};
	initCli(7, const_cast<char**>(av));

	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	impl__ArrayOf_USCOREsoapenc_USCOREstring* arr = getStatusArray(&service);
	const vector<string>& statuses = arr->item;
	BOOST_CHECK(statuses.size() == 6);
	BOOST_CHECK(statuses[0].compare("status1") == 0);
	BOOST_CHECK(statuses[1].compare("status2") == 0);
	BOOST_CHECK(statuses[2].compare("status3") == 0);
	BOOST_CHECK(statuses[3].compare("status4") == 0);
	BOOST_CHECK(statuses[4].compare("status5") == 0);
	BOOST_CHECK(statuses[5].compare("status6") == 0);
}

