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


#include "gsoap_config_proxy.h"

#include "ui/NameValueCli.h"

#include "common/InstanceHolder.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

typedef InstanceHolder<SoapBindingProxy> ServiceProxyInstanceHolder;


/**
 * This is the entry point for the fts3-config-set command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	SoapBindingProxy& service = ServiceProxyInstanceHolder::getInstance();

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
		service.soap_endpoint = endpoint.c_str();

		// TODO cgsi soap init!!!

		if (cli.isVerbose()) {
			// TODO verbose part !!!
		}

		impl__getConfigurationResponse resp;
		int err = service.getConfiguration(resp);

		if (err) {
			cout << "Failed to cancel transfer: cancel. ";
			// TODO print error message
			//manager->printSoapErr(service);
			return 0;
		}

		vector<string> &names = resp.configuration->key, &values = resp.configuration->value;
		vector<string>::iterator it_name, it_value;

		for (it_name = names.begin(), it_value = values.begin(); it_name < names.end(); it_name++, it_value++) {
			cout << *it_name << " " << *it_value << endl;
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
