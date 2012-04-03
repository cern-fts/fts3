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

#include "NameValueCli.h"

#include <stdexcept>
#include <boost/algorithm/string.hpp>

using namespace boost::algorithm;
using namespace fts3::cli;


NameValueCli::NameValueCli() {

	// add hidden options (not printed in help)
	hidden.add_options()
			("name-value", value< vector<string> >(), "Specify job name-value pairs.")
			;

	// all positional parameters go to jobid
	p.add("name-value", -1);
}

NameValueCli::~NameValueCli() {
}

void NameValueCli::initCli(int ac, char* av[]) {

	// do the basic initialization
	CliBase::initCli(ac, av);

	// parse name-value pairs
	if (vm.count("name-value")) {

		const vector<string>& pairs = vm["name-value"].as< vector<string> >();

		names.reserve(pairs.size());
		values.reserve(pairs.size());

		string name, value;

		vector<string>::const_iterator it;
		for (it = pairs.begin(); it < pairs.end(); it++) {

			size_t pos = it->find('=');
			if (pos == string::npos) {
				invalid_argument ex("Wrong name-value pair format!");
				throw ex;
			}

			name = it->substr(0, pos);
			trim(name);

			value = it->substr(pos + 1);
			trim(value);

			if (name.empty() || value.empty()) {
				invalid_argument ex("Wrong name-value pair format!");
				throw ex;
			}

			names.push_back(name);
			values.push_back(value);
		}
	}
}

string NameValueCli::getUsageString(string tool) {
	return "Usage: " + tool + " [options] NAME=VALUE [NAME=VALUE...]";
}

vector<string>& NameValueCli::getNames() {
	return names;
}

vector<string>& NameValueCli::getValues() {
	return values;
}
