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
 * fts3-transfer-getroles.cpp
 *
 *  Created on: Mar 13, 2012
 *      Author: Michal Simon
 */

#include "ServiceProxyHolder.h"
#include "ui/DNCli.h"
#include "SrvManager.h"

#include <exception>

using namespace std;
using namespace fts3::cli;


/**
 * This is the entry point for the fts3-transfer-cancel command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy& service = ServiceProxyHolder::getServiceProxy();
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
    	DNCli cli;
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

		string dn = cli.getUserDn();
		int err;
		if (dn.empty()) {
			impl__getRolesResponse resp;
			err = service.getRoles(resp);

			if (err) {
				cout << "Failed to get roles: getRoles. ";
				manager->printSoapErr(service);
				return 0;
			}

			tns3__Roles* roles = resp.getRolesReturn;

	        cout << "Your current clientDN is: " <<  *roles->clientDN << endl;

	        if (roles->serviceAdmin != 0) {

	            cout << "You are authorized as Service Administrator because your certificate contains the following principal: ";
	            cout << *roles->serviceAdmin << endl;
	        }

	        if (roles->submitter == 0) {

	            cout << "You are not authorized to submit to this service." << endl;

	        } else {

	            cout << "You are authorized to submit to this service because your certificate contains the following principal: ";
	            cout << *roles->submitter << endl;
	        }

	        cout << "You are VO manager for " << roles->VOManager.size() << " VOs." << endl;

	        std::vector<tns3__StringPair*>::iterator it;
	        for (it = roles->VOManager.begin(); it < roles->VOManager.end(); it++) {
	            cout << "You are VO manager for VO <" << *(*it)->string1;
	            cout << "> because your certificate contains the following principal: " << *(*it)->string2 << endl;
	        }

		} else {
			impl__getRolesOfResponse resp;
			err = service.getRolesOf(dn, resp);

			if (err) {
				cout << "Failed to get roles: getRolesOf. ";
				manager->printSoapErr(service);
				return 0;
			}

			tns3__Roles* roles = resp._getRolesOfReturn;

	        cout << "The DN of the user you are checking is: " << *roles->clientDN << endl;;
	        if (roles->serviceAdmin == 0) {

	            cout << "This user is not authorized as Service Administrator." << endl;

	        } else {

	            cout << "This user is authorized as Service Administrator." << endl;
	        }

	        if (roles->submitter == 0) {

	            cout << "The user is not authorized to submit to the service." << endl;

	        } else {

	            cout << "The user is authorized to submit to the service." << endl;
	        }

	        cout << "The user is VO manager for " << roles->VOManager.size() << " VOs.";

	        std::vector<tns3__StringPair*>::iterator it;
	        for (it = roles->VOManager.begin(); it < roles->VOManager.end(); it++) {
	            cout << "The user is VO manager for VO <" << *(*it)->string1 << ">" << endl;
	        }
		}



		if (dn.empty()) {


		} else {

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

