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



#include "GsoapStubs.h"
#include "ListVOManagersCli.h"
#include "SrvManager.h"
#include "evn.h"

#include <exception>
#include <string>
#include <iostream>

using namespace std;
using namespace fts3::cli;


/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy service;
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
    	ListVOManagersCli cli;
    	cli.initCli(ac, av);

    	// if applicable print help or version and exit
		if (cli.printHelp(av[0]) || cli.printVersion()) return 0;

		// get the source file, the destination file and the FTS3 service endpoint
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
		if (manager->init(service)) return 0;

		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		string vo  = cli.getVOName();
		if (vo.empty()) {
			cout << "The VO name has to be specified" << endl;
			return 0;
		}

		int err;
		fts__listVOManagersResponse resp;
		err = service.listVOManagers(vo, resp);

		if (err) {
			cout << "Failed to list VO managers transfer: listVOManagers. ";
			manager->printSoapErr(service);
			return 0;
		}

		vector<string> &vec = resp._listVOManagersReturn->item;
		vector<string>::iterator it;

		for (it = vec.begin(); it < vec.end(); it++) {
			cout << *it << endl;
		}
	}
	catch(std::exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch(...) {
		cerr << "Exception of unknown type!\n";
	}

	return 0;
}
