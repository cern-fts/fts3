/*
 * ListTransferCli.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: simonm
 */

#include "ListTransferCli.h"
#include "SrvManager.h"

using namespace fts::cli;

ListTransferCli::ListTransferCli() {

	// set status names
	FTS3_STATUS_ACTIVE = "Active";
	FTS3_STATUS_PENDING = "Pending";
	FTS3_STATUS_READY = "Ready";
	FTS3_STATUS_SUBMITTED = "Submitted";

	// add fts3-transfer-status specific options
	specific.add_options()
			("userdn,u", value<string>(), "Restrict to specific user DN.")
			("voname,o", value<string>(), "Restrict to specific VO")
			;

	// add hidden options (not printed in help)
	hidden.add_options()
			("state", value< vector<string> >(), "Specify states for querying.")
			;

	// all positional parameters go to state
	p.add("state", -1);

	// add specific and hidden parameters to all parameters
	all.add(specific).add(hidden);
	// add specific parameters to visible parameters (printed in help)
	visible.add(specific);
}

ListTransferCli::~ListTransferCli() {
	// TODO Auto-generated destructor stub
}

string ListTransferCli::getUsageString() {
	return "Usage: fts3-transfer-list [options] [STATE...]";
}

string ListTransferCli::getVoname() {

	if (vm.count("voname")) {
		return vm["voname"].as<string>();
	}

	return string();
}

string ListTransferCli::getUserdn() {

	if (vm.count("userdn")) {
		return vm["userdn"].as<string>();
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
