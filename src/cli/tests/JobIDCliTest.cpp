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
#include "ui/JobIDCli.h"
#include "unittest/testsuite.h"

#include <string>
#include <vector>

using namespace std;
using namespace fts3::cli;

BOOST_FIXTURE_TEST_CASE (JobIDCli_Test1, JobIDCli) {

	// has to be const otherwise is deprecated
	const char* av[] = {"prog_name", "ID1", "ID2", "ID3"};
	initCli(4, const_cast<char**>(av));
	// there should be a possitional parameter
	BOOST_CHECK(vm.count("jobid"));

	const vector<string>& ids = getJobIds();

	// the vector should have 3 elements
	BOOST_CHECK(ids.size() == 3);
	// check if the values are correct
	BOOST_CHECK(ids[0].compare("ID1") == 0);
	BOOST_CHECK(ids[1].compare("ID2") == 0);
	BOOST_CHECK(ids[2].compare("ID3") == 0);
}
