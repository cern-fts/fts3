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
#include "TransferStatusCli.h"
#include "SrvManager.h"
#include <cgsi_plugin.h>
#include "evn.h"

#include <vector>
#include <string>

using namespace std;
using namespace fts::cli;


/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[]) {

	// create the service client
	FileTransferSoapBindingProxy service;
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and the command line utility
		TransferStatusCli cli;
		cli.initCli(ac, av);

		// if applicable print help or version and exit
		if (cli.printHelp() || cli.printVersion()) return 0;

		// get the FTS3 service endpoint
		string endpoint = cli.getService();

		// check if its not empty
		if (endpoint.empty()) {
			cout << "The service has not been defined." << endl;
			return 0;
		}

		// get job IDs that have to be check
		vector<string> jobIds = cli.getJobIds();
		if (jobIds.empty()) {
			cout << "No request ID specified." << endl;
			return 0;
		}

		//service.soap_endpoint = "https://vtb-generic-32.cern.ch:8443/glite-data-transfer-fts/services/FileTransfer";
		// set the endpoint
		service.soap_endpoint = endpoint.c_str();

		// initialize SOAP
		if (!manager->initSoap(&service, endpoint)) return 0;

		// check the interface version of the FTS3 service
		fts__getInterfaceVersionResponse ivresp;
		service.getInterfaceVersion(ivresp);
		string interface = ivresp.getInterfaceVersionReturn;

		// set the interface version
		manager->setInterfaceVersion(interface);
		/*
		// initialize SrvManager
		if (manager->init(service)) {
			cout << "Error while init SrvManager." << endl;
		}
		*/

		// if verbose print general info
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		// iterate over job IDs
		vector<string>::iterator it;
		for (it = jobIds.begin(); it < jobIds.end(); it++) {

			//string job = "1215375b-5407-11e1-9322-bc3d2ed9263b"
			string jobId = *it;
			int ret;

			if (cli.isVerbose()) {

				if (manager->isItVersion330()) {
					// if version higher than 3.3.0 use getTransferJobSummary2

					// do the request
					fts__getTransferJobSummary2Response resp;
					ret = service.getTransferJobSummary2(jobId, resp);

					// print the response
					if (!ret) {
						cout << "Request ID: " << jobId << endl;
						cout << "Status: " << *resp._getTransferJobSummary2Return->jobStatus->jobStatus << endl;
						cout << "Client DN: " << *resp._getTransferJobSummary2Return->jobStatus->clientDN << endl;

						if (resp._getTransferJobSummary2Return->jobStatus->reason) {
							cout << "Reason: " << *resp._getTransferJobSummary2Return->jobStatus->reason << endl;
						} else {
							cout << "Reason: <None>" << endl;
						}

						cout << "Submit time: " << ""/*TODO*/ << endl;
						cout << "Files: " << resp._getTransferJobSummary2Return->jobStatus->numFiles << endl;
					    cout << "Priority: " << resp._getTransferJobSummary2Return->jobStatus->priority << endl;
					    cout << "VOName: " << *resp._getTransferJobSummary2Return->jobStatus->voName << endl;

					    cout << "\tDone: " << resp._getTransferJobSummary2Return->numDone << endl;
					    cout << "\tActive: " << resp._getTransferJobSummary2Return->numActive << endl;
					    cout << "\tPending: " << resp._getTransferJobSummary2Return->numPending << endl;
					    cout << "\tReady: " << resp._getTransferJobSummary2Return->numReady << endl;
					    cout << "\tCanceled: " << resp._getTransferJobSummary2Return->numCanceled << endl;
					    cout << "\tFailed: " << resp._getTransferJobSummary2Return->numFailed << endl;
					    cout << "\tFinishing: " << resp._getTransferJobSummary2Return->numFinishing << endl;
					    cout << "\tFinished: " << resp._getTransferJobSummary2Return->numFinished << endl;
					    cout << "\tSubmitted: " << resp._getTransferJobSummary2Return->numSubmitted << endl;
					    cout << "\tHold: " << resp._getTransferJobSummary2Return->numHold << endl;
					    cout << "\tWaiting: " << resp._getTransferJobSummary2Return->numWaiting << endl;
					}

				} else {
					// if version higher than 3.3.0 use getTransferJobSummary

					// do the request
					fts__getTransferJobSummaryResponse resp;
					ret = service.getTransferJobSummary(jobId, resp);

					// print the response
					if (!ret) {
						cout << "Request ID: " << jobId << endl;
						cout << "Status: " << *resp._getTransferJobSummaryReturn->jobStatus->jobStatus << endl;
						cout << "Client DN: " << *resp._getTransferJobSummaryReturn->jobStatus->clientDN << endl;

						if (resp._getTransferJobSummaryReturn->jobStatus->reason) {
							cout << "Reason: " << *resp._getTransferJobSummaryReturn->jobStatus->reason << endl;
						} else {
							cout << "Reason: <None>" << endl;
						}

						cout << "Submit time: " << ""/*TODO*/ << endl;
						cout << "Files: " << resp._getTransferJobSummaryReturn->jobStatus->numFiles << endl;
					    cout << "Priority: " << resp._getTransferJobSummaryReturn->jobStatus->priority << endl;
					    cout << "VOName: " << *resp._getTransferJobSummaryReturn->jobStatus->voName << endl;

					    cout << "\tDone: " << resp._getTransferJobSummaryReturn->numDone << endl;
					    cout << "\tActive: " << resp._getTransferJobSummaryReturn->numActive << endl;
					    cout << "\tPending: " << resp._getTransferJobSummaryReturn->numPending << endl;
					    cout << "\tCanceled: " << resp._getTransferJobSummaryReturn->numCanceled << endl;
					    cout << "\tCanceling: " << resp._getTransferJobSummaryReturn->numCanceling << endl;
					    cout << "\tFailed: " << resp._getTransferJobSummaryReturn->numFailed << endl;
					    cout << "\tFinished: " << resp._getTransferJobSummaryReturn->numFinished << endl;
					    cout << "\tSubmitted: " << resp._getTransferJobSummaryReturn->numSubmitted << endl;
					    cout << "\tHold: " << resp._getTransferJobSummaryReturn->numHold << endl;
					    cout << "\tWaiting: " << resp._getTransferJobSummaryReturn->numWaiting << endl;
					    cout << "\tCatalogFailed: " << resp._getTransferJobSummaryReturn->numCatalogFailed << endl;
					}
				}
			} else {

				// do the request
				fts__getTransferJobStatusResponse resp;
		    	ret = service.getTransferJobStatus(jobId, resp);

		    	// print the response
		    	if (!ret) {
		    		cout << *resp._getTransferJobStatusReturn->jobStatus << endl;
		    	}
			}

			// TODO test!
			// check if the -l option has been used
			if (cli.list()) {

				// do the request
				fts__getFileStatusResponse resp;
				ret = service.getFileStatus(jobId, 0, 100, resp);

				if (!ret) {

					std::vector<transfer__FileTransferStatus * >& vect = resp._getFileStatusReturn->item;
					std::vector<transfer__FileTransferStatus * >::iterator it;

					// print the response
					for (it = vect.begin(); it < vect.end(); it++) {
						transfer__FileTransferStatus* stat = *it;

						cout << "  Source:      " << *stat->sourceSURL << endl;
						cout << "  Destination: " << *stat->destSURL << endl;
						cout << "  State:       " << *stat->transferFileState << endl;;
						cout << "  Retries:     " << stat->numFailures << endl;
						cout << "  Reason:      " << *stat->reason << endl;
						cout << "  Duration:    " << stat->duration << endl;
					}
				}
			}

			// print the error message if applicable
	    	if (ret) {
	    		if (service.fault) {
	    			if (service.fault->detail) {
	    				if (service.fault->detail->fault) {
	    		    		transfer__TransferException* ex = (transfer__TransferException*)service.fault->detail->fault;
    		    			cout << "getTransferJobStatus: " << ex->message << endl;
  						}
	    			}
	    		}

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
