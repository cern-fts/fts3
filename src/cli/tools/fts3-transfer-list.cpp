/*
 * fts3-transfer-list.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: simonm
 */


#include "ServiceProxyHolder.h"
#include "ui/ListTransferCli.h"
#include "SrvManager.h"
#include "common/JobStatusHandler.h"

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy& service = ServiceProxyHolder::getServiceProxy();
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
		if (!manager->init(service)) return 0;


		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		if (!cli.checkIfFeaturesSupported()) {
			return 0;
		}

		impl__ArrayOf_USCOREsoapenc_USCOREstring* array = cli.getStatusArray(&service);
		int err;

		vector<tns3__JobStatus * > statuses;

		if (manager->isUserVoRestrictListingSupported()) {

			impl__listRequests2Response resp;
			err = service.listRequests2(array, cli.getUserDn(), cli.getVOName(), resp);

			if (err || !resp._listRequests2Return) {
				cout << "Failed to list transfers: ListRequest2. ";
				manager->printSoapErr(service);
				return 0;
			}

			statuses = resp._listRequests2Return->item;

		} else {

			impl__listRequestsResponse resp;
			err = service.listRequests(array, resp);

			if (err || !resp._listRequestsReturn) {
				cout << "Failed to list transfers: ListRequest. ";
				manager->printSoapErr(service);
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

	return 0;
}
