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
 * NameValueCli.cpp
 *
 *  Created on: Apr 2, 2012
 *      Author: Michał Simon
 */

#include "CfgCli.h"

#include <stdexcept>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;
using namespace fts3::cli;


CfgCli::CfgCli() {

	// add hidden options (not printed in help)
	hidden.add_options()
			("cfg", value< vector<string> >(), "Specify SE configuration.")
			;

	// all positional parameters go to jobid
	p.add("cfg", -1);
}

CfgCli::~CfgCli() {
}

GSoapContextAdapter* CfgCli::validate() {

	if (!CliBase::validate()) return 0;

	if (getConfigurations().empty()) {
		cout << "No parameters have been specified." << endl;
		return 0;
	}

	return ctx;
}

string CfgCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] CONFIG [CONFIG...]";
}

vector<string> CfgCli::getConfigurations() {

	// check whether jobid has been given as a parameter
	if (vm.count("cfg")) {
		return vm["cfg"].as< vector<string> >();
	}

	return vector<string>();
}
