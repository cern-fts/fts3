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
#include "SubmitTransferCli.h"
#include "SrvManager.h"
#include "evn.h"

using namespace std;
using namespace fts::cli;


int main(int ac, char* av[]) {

	// create service
	FileTransferSoapBindingProxy service;
	/*service.soap_endpoint = "localhost:8443/glite-data-transfer-interface/FileTransfer";


	transfer__TransferJob job;
	fts__transferSubmit2Response resp;
	int ret = service.transferSubmit2(&job, resp);
	cout << "Submitted request ID: " << resp._transferSubmit2Return << endl;

	return 0;*/
	// Srv manager
	SrvManager* manager = SrvManager::getInstance();

	try {
    	SubmitTransferCli cli;
    	cli.initCli(ac, av);

		if (cli.printHelp() || cli.printVersion()) return 0;

    	string source = cli.getSource(), destination = cli.getDestination(), endpoint = cli.getService();

		// set endpoint
		if (endpoint.size() == 0) {
			cout << "Failed to determine the endpoint" << endl;
			return 0;
		}
		service.soap_endpoint = endpoint.c_str();

		// initialize SOAP
		if (!manager->initSoap(&service, endpoint)) return 0;

		// initialize SrvManager
		if (manager->init(service)) {
			cout << "Error while init SrvManager." << endl;
		}

		// produce output
		if (cli.isVerbose()) {
			cli.printGeneralInfo();
		}

		if(!cli.performChecks()) return 0;
		// JobElement
		if (!cli.createJobElements()) return 0;

		string jobId;

		if (cli.useCheckSum()) {
			transfer__TransferJob2 job;
			// Elements
			job.transferJobElements = cli.getJobElements2(&service);
			// Params
			job.jobParams = cli.getParams(&service);

			// always use delegation with checksum TODO check whether it's right!
			manager->delegateProxyCert(endpoint);
			// transfer submit
			fts__transferSubmit3Response resp;
			int ret = service.transferSubmit3(&job, resp);

			jobId = resp._transferSubmit3Return;

		} else {
			// Job
			transfer__TransferJob job;
			// Elements
			job.transferJobElements = cli.getJobElements(&service);
			// Params
			job.jobParams = cli.getParams(&service);

			if (cli.useDelegation()) {

				manager->delegateProxyCert(endpoint);
				// transfer submit
				fts__transferSubmit2Response resp;
				int ret = service.transferSubmit2(&job, resp);

				jobId = resp._transferSubmit2Return;

			} else {

				job.credential = soap_new_std__string(&service, -1);
				*job.credential = cli.getPassword(); // TODO test
				cout << *job.credential << endl;

				// transfer submit
				fts__transferSubmitResponse resp;
				int ret = service.transferSubmit(&job, resp);

				jobId = resp._transferSubmitReturn;
			}
		}

		cout << "Submitted request ID: " << jobId << endl;

		if (cli.isBlocking()) {
			fts__getTransferJobStatusResponse resp;
			do {
				sleep(2);
				service.getTransferJobStatus(jobId, resp);
			} while (!manager->isTransferReady(*resp._getTransferJobStatusReturn->jobStatus));
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
