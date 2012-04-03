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
 * DnCliTest.cpp
 *
 *  Created on: Mar 19, 2012
 *      Author: Michał Simon
 */


//#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/DnCli.h"
#include "unittest/testsuite.h"

using namespace fts3::cli;


BOOST_FIXTURE_TEST_CASE (DnCli_Test1, DnCli) {
	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name", "-u", "userdn"};
	initCli(3, const_cast<char**>(av));
	// all 5 parameters should be available in vm variable
	BOOST_CHECK(vm.count("userdn"));
	// the endpoint shouldn't be empty since it's starting with http
	BOOST_CHECK(getUserDn().compare("userdn") == 0);
}

BOOST_FIXTURE_TEST_CASE (DnCli_Test2, DnCli) {
	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name", "--userdn", "userdn"};
	initCli(3, const_cast<char**>(av));
	// all 5 parameters should be available in vm variable
	BOOST_CHECK(vm.count("userdn"));
	// the endpoint shouldn't be empty since it's starting with http
	BOOST_CHECK(getUserDn().compare("userdn") == 0);
}
