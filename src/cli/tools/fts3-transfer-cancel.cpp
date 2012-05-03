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
 * fts3-config-get.cpp
 *
 *  Created on: Apr 3, 2012
 *      Author: Michał Simon
 */

#include "gsoap_proxy.h"
#include "SrvManager.h"
#include "ui/JobIdCli.h"

#include "common/InstanceHolder.h"

#include <exception>
#include <iostream>
#include <vector>
#include <string>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;


typedef InstanceHolder<FileTransferSoapBindingProxy> ServiceProxyInstanceHolder;

/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
    	JobIdCli cli;
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
		service.soap_endpoint = endpoint.c_str();

		// initialize SOAP
		if (!manager->initSoap(&service, endpoint)) return 0;

		// initialize SrvManager
		if (!manager->init(service)) return 0;

		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		impltns__ArrayOf_USCOREsoapenc_USCOREstring* rqst = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(&service, -1);
		vector<string> &jobs = rqst->item;
		jobs = cli.getJobIds();

		if (jobs.empty()) {
			cout << "No request ID has been specified" << endl;
			return 0;
		}

		impltns__cancelResponse resp;
		int err = service.cancel(rqst, resp);

		if (err) {
			cout << "Failed to cancel transfer: cancel. ";
			manager->printSoapErr(service);
			return 0;
		}

		vector<string>::iterator it;

	    for (it = jobs.begin(); it < jobs.end(); it++) {
	        cout << "Canceled " << *it << endl;;
	    }

	} catch(std::exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch(...) {
		cerr << "Exception of unknown type!\n";
	}

	return 0;
}
