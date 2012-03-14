/*
 * fts3-transfer-list.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: simonm
 */


#include "GsoapStubs.h"
#include "ListTransferCli.h"
#include "SrvManager.h"
#include "evn.h"

using namespace std;
using namespace fts3::cli;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy service;
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
		service.soap_endpoint = endpoint.c_str();

		// initialize SOAP
		if (!manager->initSoap(&service, endpoint)) return 0;

		// initialize SrvManager
		if (manager->init(service)) return 0;


		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		if (!cli.checkIfFeaturesSupported()) {
			return 0;
		}

		fts__ArrayOf_USCOREsoapenc_USCOREstring* array = cli.getStatusArray(&service);
		int err;

		vector<transfer__JobStatus * > statuses;

		if (manager->isUserVoRestrictListingSupported()) {

			fts__listRequests2Response resp;
			err = service.listRequests2(array, cli.getUserDn(), cli.getVoname(), resp);

			if (err) {
				//TODO err handle
				return 0;
			}

			statuses = resp._listRequests2Return->item;
		} else {

			fts__listRequestsResponse resp;
			err = service.listRequests(array, resp);

			if (err) {
				//TODO err handle
				return 0;
			}

			statuses = resp._listRequestsReturn->item;
		}

		vector<transfer__JobStatus * >::iterator it;
		for (it = statuses.begin(); it < statuses.end(); it++) {
			if (cli.isVerbose()) {

				// TODO make common print for transfer__JobStatus (also used in fts3-transfer-status)!
				cout << "Request ID:\t" << *(*it)->jobID << endl;
				cout << "Status: " << *(*it)->jobStatus << endl;
				cout << "Client DN: " << *(*it)->clientDN << endl;

				if ((*it)->reason) {
					cout << "Reason: " << (*it)->reason << endl;
				} else {
					cout << "Reason: <None>" << endl;
				}

				cout << "Submit time: " << ""/*TODO*/ << endl;
				cout << "Files: " << (*it)->numFiles << endl;
			    cout << "Priority: " << (*it)->priority << endl;
			    cout << "VOName: " << *(*it)->voName << endl;

			} else {
				cout << *(*it)->jobID << "\t" << *(*it)->jobStatus << endl;
			}
		}

	}
	catch(exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch(...) {
		cerr << "Exception of unknown type!\n";
	}

	return 0;
}
