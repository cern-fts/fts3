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

#include "TransferTypes.h"

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
		optional<GSoapContextAdapter&> opt = cli->validate();
		if (!opt.is_initialized()) return 0;
		GSoapContextAdapter& ctx = opt.get();

		// get the source file, the destination file and the FTS3 service endpoint
    	string source = cli->getSource(), destination = cli->getDestination();

		string jobId;

		if (cli->useDelegation()) {

			// delegate Proxy Certificate
			ProxyCertificateDelegator handler (
					cli->getService(),
					cli->getDelegationId(),
					cli->getExpirationTime()
				);

			if (cli->isVerbose()) {
				handler.delegate();
			} else {
				cli->mute();
				handler.delegate();
				cli->unmute();
			}

			// submit the job
			jobId = ctx.transferSubmit (
					cli->getJobElements(),
					cli->getParams(),
					cli->useCheckSum()
				);

		} else {
			// submit the job
			jobId = ctx.transferSubmit (
					cli->getJobElements(),
					cli->getParams(),
					cli->getPassword()
				);
		}

		cout << jobId << endl;

		// check if the -b option has been used
		if (cli->isBlocking()) {

			fts3::cli::JobStatus status;
			// wait until the transfer is ready
			do {
				sleep(2);
				status = ctx.getTransferJobStatus(jobId);
			} while (!JobStatusHandler::getInstance().isTransferReady(status.jobStatus));
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
