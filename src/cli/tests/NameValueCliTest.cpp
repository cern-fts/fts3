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
 * NameValueCliTest.cpp
 *
 *  Created on: Apr 16, 2012
 *      Author: Michal Simon
 */




//#ifdef FTS3_COMPILE_WITH_UNITTEST
#include "ui/NameValueCli.h"
#include "unittest/testsuite.h"

#include <stdexcept>
#include <string>
#include <vector>

using namespace fts3::cli;
using namespace std;

BOOST_FIXTURE_TEST_CASE (NameValueCli_WrongFormat_Test, NameValueCli) {

        // has to be const otherwise is deprecated
        const char* av[] = {"prog_name", "wrong_format"};

        bool wrong_format = false;

        try {
        	initCli(2, const_cast<char**>(av));
        } catch (invalid_argument& e) {
        	wrong_format = true;
		}

        BOOST_CHECK(wrong_format);
}

BOOST_FIXTURE_TEST_CASE (NameValueCli_Test, NameValueCli) {

        // has to be const otherwise is deprecated
        const char* av[] = {"prog_name", "p1=1", "p2=2", "p3=3"};

       	initCli(4, const_cast<char**>(av));

       	vector<string> &names = getNames(), &values = getValues();

       	BOOST_CHECK(names.size() == 3 && values.size() == 3);

       	BOOST_CHECK(names[0] == "p1" && values[0] == "1");
       	BOOST_CHECK(names[1] == "p2" && values[1] == "2");
       	BOOST_CHECK(names[2] == "p3" && values[2] == "3");
}
