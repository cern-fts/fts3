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
#include "common/JobStatusHandler.h"

#include <exception>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-submit command line tool.
 */
int main(int ac, char* av[]) {

	// create FTS3 service client
	FileTransferSoapBindingProxy service;
	// get SrvManager instance
	SrvManager* manager = SrvManager::getInstance();

	try {
		// create and initialize the command line utility
    	SubmitTransferCli cli;
    	cli.initCli(ac, av);

    	// if applicable print help or version and exit
		if (cli.printHelp(av[0]) || cli.printVersion()) return 0;

		// get the source file, the destination file and the FTS3 service endpoint
    	string source = cli.getSource(), destination = cli.getDestination(), endpoint = cli.getService();

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

		// perform standard checks in order to determine if the job was well specified
		if(!cli.performChecks()) return 0;

		// prepare job elements
		if (!cli.createJobElements()) return 0;

		string jobId;
		int err;

		// checksum requires different request
		if (cli.useCheckSum()) {
			transfer__TransferJob2 job;
			// set job elements
			job.transferJobElements = cli.getJobElements2(&service);
			// set transfer parameters
			job.jobParams = cli.getParams(&service);

			// always use delegation with checksum TODO check whether it's right!
			manager->delegateProxyCert(endpoint);
			// submit the job
			fts__transferSubmit3Response resp;
			err = service.transferSubmit3(&job, resp);
			if (err) {
				cout << "Failed to submit transfer: transferSubmit3. ";
				manager->printSoapErr(service);
				return 0;
			}

			// retrieve the job ID
			jobId = resp._transferSubmit3Return;

		} else {
			transfer__TransferJob job;
			// set job elements
			job.transferJobElements = cli.getJobElements(&service);
			// set transfer parameters
			job.jobParams = cli.getParams(&service);

			// check whether a proxy certificate should be used
			if (cli.useDelegation()) {

				manager->delegateProxyCert(endpoint);
				// submit the job
				fts__transferSubmit2Response resp;
				err = service.transferSubmit2(&job, resp);
				if (err) {
					cout << "Failed to submit transfer: transferSubmit2. ";
					manager->printSoapErr(service);
					return 0;
				}

				// retrieve the job ID
				jobId = resp._transferSubmit2Return;

			} else {

				// set the credential (Password)
				job.credential = soap_new_std__string(&service, -1);
				*job.credential = cli.getPassword(); // TODO test
				//cout << *job.credential << endl;

				// submit the job
				fts__transferSubmitResponse resp;
				err = service.transferSubmit(&job, resp);
				if (err) {
					cout << "Failed to submit transfer: transferSubmit. ";
					manager->printSoapErr(service);
					return 0;
				}

				// retrieve the job ID
				jobId = resp._transferSubmitReturn;
			}
		}

		cout << "Submitted request ID: " << jobId << endl;

		// check if the -b option has been used
		if (cli.isBlocking()) {
			fts__getTransferJobStatusResponse resp;
			// wait until the transfer is ready
			do {
				sleep(2);
				service.getTransferJobStatus(jobId, resp);
			} while (!JobStatusHandler::getInstance().isTransferReady(*resp._getTransferJobStatusReturn->jobStatus));
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
