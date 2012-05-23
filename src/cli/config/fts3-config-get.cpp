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
#include "ui/CliBase.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;
using namespace fts3::cli;

/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[]) {

	soap* soap = soap_new();
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
		CliBase cli;
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

		// TODO cgsi soap init!!!
		if (cli.isVerbose()) {
			// TODO verbose part !!!
		}

		implcfg__getConfigurationResponse resp;
		int err = soap_call_implcfg__getConfiguration(soap, endpoint.c_str(), 0, resp);
				//service.getConfiguration(resp);

		if (err) {
			cout << "Failed to get configuration. " << endl;;
			// TODO print error message
			//manager->printSoapErr(service);
			return 0;
		}

		vector<string> &cfgs = resp.configuration->cfg;
		vector<string>::iterator it;

		for (it = cfgs.begin(); it < cfgs.end(); it++) {
			cout << *it << endl;
		}

	} catch(std::exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch(...) {
		cerr << "Exception of unknown type!\n";
	}

	soap_destroy(soap);
	soap_end(soap);
	soap_free(soap);

	return 0;
}
