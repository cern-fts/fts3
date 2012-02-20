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
 */

/*
 * TransferStatusCli.cpp
 *
 *  Created on: Feb 13, 2012
 *      Author: simonm
 */

#include "TransferStatusCli.h"
#include <vector>

using namespace std;
using namespace fts::cli;

TransferStatusCli::TransferStatusCli() {
	specific.add_options()
			("list,l", "List status for all files.")
			;

	hidden.add_options()
			("jobid", value< vector<string> >(), "Specify source site name.")
			;

	p.add("jobid", -1);

	cli_options.add(specific).add(hidden);
	visible.add(specific);
}

TransferStatusCli::~TransferStatusCli() {
}

string TransferStatusCli::getUsageString() {
	return "Usage: glite-transfer-status [options] JOBID [JOBID...]";
}

vector<string> TransferStatusCli::getJobIds() {
	if (vm.count("jobid")) {
		return vm["jobid"].as< vector<string> >();
	}

	vector<string> empty;
	return empty;
}

bool TransferStatusCli::list() {

	return vm.count("list");
}
