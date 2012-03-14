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
 * ListTransferCli.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: Michal Simon
 */

#include "ListTransferCli.h"
#include "SrvManager.h"

using namespace fts3::cli;

ListTransferCli::ListTransferCli() {

	// set status names
	FTS3_STATUS_ACTIVE = "ACTIVE";
	FTS3_STATUS_PENDING = "PENDING";
	FTS3_STATUS_READY = "READY";
	FTS3_STATUS_SUBMITTED = "SUBMITTED";

	// add fts3-transfer-status specific options
	specific.add_options()
			("voname,o", value<string>(), "Restrict to specific VO")
			;

	// add hidden options (not printed in help)
	hidden.add_options()
			("state", value< vector<string> >(), "Specify states for querying.")
			;

	// all positional parameters go to state
	p.add("state", -1);
}

ListTransferCli::~ListTransferCli() {
}

string ListTransferCli::getUsageString(string tool) {

	return "Usage: " + tool + " [options] [STATE...]";
}

string ListTransferCli::getVoname() {

	if (vm.count("voname")) {
		return vm["voname"].as<string>();
	}

	return string();
}

bool ListTransferCli::checkIfFeaturesSupported() {

	SrvManager* manager = SrvManager::getInstance();

	if (vm.count("userdn") && !manager->isUserVoRestrictListingSupported()) {
		cout << "The server you are contacting does not support the -u option (it is running interface version ";
		cout << manager->getInterface() << ")." << endl;
		return false;
	}

    if ( vm.count("voname") && !manager->isUserVoRestrictListingSupported()) {
        cout << "The server you are contacting does not support the -o option (it is running interface version ";
        cout << manager->getInterface() << ")." << endl;
        return false;
    }

   return true;
}

fts__ArrayOf_USCOREsoapenc_USCOREstring* ListTransferCli::getStatusArray(soap* soap) {

	fts__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_fts__ArrayOf_USCOREsoapenc_USCOREstring(soap, -1);

	if (vm.count("state")) {
		array->item = vm["state"].as<vector<string> >();
	}

	if (array->item.empty()) {
		array->item.push_back(FTS3_STATUS_SUBMITTED);
		array->item.push_back(FTS3_STATUS_PENDING);
		array->item.push_back(FTS3_STATUS_ACTIVE);

		if (SrvManager::getInstance()->isUserVoRestrictListingSupported()) {
			array->item.push_back(FTS3_STATUS_READY);
		}
	}

	return array;
}
