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
 * CliBaseTest.cpp
 *
 *  Created on: Feb 23, 2012
 *      Author: Michal Simon
 */
#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "CliBase.h"
#include "unittest/testsuite.h"
#include "evn.h"

using namespace fts::cli;


/*
 * Dummy class that inherits after abstract CliBase
 * and implements its pure virtual method so the class
 * can be instantiated and tested
 */
class CliBaseTester : public CliBase {

	// implement the pure vitual method
	string getUsageString() {return "";};
};

BOOST_FIXTURE_TEST_CASE (CliBase_Test1, CliBaseTester) {

	char* av[] = {"prog_name", "-h", "-q", "-v", "-s", "http://hostname:1234/service", "-V"};
	initCli(7, av);
	// all 5 parameters should be available in vm variable
	BOOST_CHECK(vm.count("help") && vm.count("quite") && vm.count("verbose") && vm.count("service") && vm.count("version"));
	// the endpoint shouldn't be empty since it's starting with http
	BOOST_CHECK(!getService().empty());
}

BOOST_FIXTURE_TEST_CASE (CliBase_Test2, CliBaseTester) {

	char* av[] = {"prog_name", "--help", "--quite", "--verbose", "--service", "hostname:1234/service", "--version"};
	initCli(7, av);
	// all 5 parameters should be available in vm variable
	BOOST_CHECK(vm.count("help") && vm.count("quite") && vm.count("verbose") && vm.count("service") && vm.count("version"));
	// the endpoint should be empty since it's not starting with http, https, httpd
	BOOST_CHECK(getService().empty());
}


#endif // FTS3_COMPILE_WITH_UNITTESTS
