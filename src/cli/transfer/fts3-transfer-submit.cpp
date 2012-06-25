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

#include "GSoapContextAdapter.h"
#include "ProxyCertificateDelegator.h"
#include "ui/SubmitTransferCli.h"

#include "common/JobStatusHandler.h"

#include <exception>
#include <memory>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-submit command line tool.
 */
int main(int ac, char* av[]) {

	try {
		// create and initialize the command line utility
		auto_ptr<SubmitTransferCli> cli (
				getCli<SubmitTransferCli>(ac, av)
			);

		// validate command line options, and return respective gsoap context
		GSoapContextAdapter* ctx = cli->validate();
		if (!ctx) return 0;

		// get the source file, the destination file and the FTS3 service endpoint
    	string source = cli->getSource(), destination = cli->getDestination();

		string jobId;

		// checksum requires different request
		if (cli->useCheckSum()) {
			tns3__TransferJob2 job;
			// set job elements
			job.transferJobElements = cli->getJobElements2();
			// set transfer parameters
			job.jobParams = cli->getParams();

			// always use delegation with checksum TODO check whether it's right!
			// delegate Proxy Certificate
			ProxyCertificateDelegator handler(cli->getService(), cli->getDelegationId(), cli->getExpirationTime());
			if (!handler.delegate()) return 0;

			// submit the job
			impltns__transferSubmit3Response resp;
			ctx->transferSubmit3(&job, resp);

			// retrieve the job ID
			jobId = resp._transferSubmit3Return;

		} else {
			tns3__TransferJob job;
			// set job elements
			job.transferJobElements = cli->getJobElements();
			// set transfer parameters
			job.jobParams = cli->getParams();

			// check whether a proxy certificate should be used
			if (cli->useDelegation()) {

				// delegate Proxy Certificate
				ProxyCertificateDelegator handler(cli->getService(), cli->getDelegationId(), cli->getExpirationTime());
				if (!handler.delegate()) return 0;

				// submit the job
				impltns__transferSubmit2Response resp;
				ctx->transferSubmit2(&job, resp);

				// retrieve the job ID
				jobId = resp._transferSubmit2Return;

			} else {

				// set the credential (Password)
				job.credential = soap_new_std__string(*ctx, -1);
				*job.credential = cli->getPassword();

				// submit the job
				impltns__transferSubmitResponse resp;
				ctx->transferSubmit(&job, resp);

				// retrieve the job ID
				jobId = resp._transferSubmitReturn;
			}
		}

		cout << "Submitted request ID: " << jobId << endl;

		// check if the -b option has been used
		if (cli->isBlocking()) {
			impltns__getTransferJobStatusResponse resp;
			// wait until the transfer is ready
			do {
				sleep(2);
				ctx->getTransferJobStatus(jobId, resp);
			} while (!JobStatusHandler::getInstance().isTransferReady(*resp._getTransferJobStatusReturn->jobStatus));
		}

    } catch(std::exception& e) {
        cerr << "error: " << e.what() << "\n";
        return 1;
    } catch(string& ex) {
    	cout << ex << endl;
    	return 1;
    } catch(...) {
        cerr << "Exception of unknown type!\n";
        return 1;
    }

    return 0;
}
