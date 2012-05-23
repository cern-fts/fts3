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
 * fts3-transfer-list.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: Michal Simon
 */


#include "gsoap_proxy.h"
#include "SrvManager.h"
#include "ui/ListTransferCli.h"

#include "common/JobStatusHandler.h"

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[]) {

	soap* soap = soap_new();

	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
		ListTransferCli cli;
		cli.initCli(ac, av);

    	// if applicable print help or version and exit
		if (cli.printHelp(av[0]) || cli.printVersion()) return 0;

		// get the FTS3 service endpoint
    	string endpoint = cli.getService();

		// set the  endpoint
		if (endpoint.size() == 0) {
			cout << "Failed to determine the endpoint" << endl;
			return 0;
		}

		// initialize SOAP
		if (!manager->initSoap(soap, endpoint)) return 0;

		// initialize SrvManager
		if (!manager->init(soap, endpoint.c_str())) return 0;


		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		if (!cli.checkIfFeaturesSupported()) {
			return 0;
		}

		impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = cli.getStatusArray(soap);
		int err;

		vector<tns3__JobStatus * > statuses;

		if (manager->isUserVoRestrictListingSupported()) {

			impltns__listRequests2Response resp;
			err = soap_call_impltns__listRequests2(soap, endpoint.c_str(), 0, array, cli.getUserDn(), cli.getVoName(), resp);

			if (err || !resp._listRequests2Return) {
				cout << "Failed to list transfers: ListRequest2. ";
				manager->printSoapErr(soap);
				return 0;
			}

			statuses = resp._listRequests2Return->item;

		} else {

			impltns__listRequestsResponse resp;
			err = soap_call_impltns__listRequests(soap, endpoint.c_str(), 0, array, resp);

			if (err || !resp._listRequestsReturn) {
				cout << "Failed to list transfers: ListRequest. ";
				manager->printSoapErr(soap);
				return 0;
			}

			statuses = resp._listRequestsReturn->item;
		}

		vector<tns3__JobStatus * >::iterator it;
		for (it = statuses.begin(); it < statuses.end(); it++) {
			if (cli.isVerbose()) {

				JobStatusHandler::printJobStatus(*it);

			} else {
				cout << *(*it)->jobID << "\t" << *(*it)->jobStatus << endl;
			}
		}

	} catch(std::exception& e) {

		cerr << "error: " << e.what() << "\n";
		return 1;

	} catch(...) {

		cerr << "Exception of unknown type!\n";
	}

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}
