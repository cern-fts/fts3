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
#include "ui/TransferStatusCli.h"

#include "common/JobStatusHandler.h"

#include <vector>
#include <string>
#include <memory>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;

/**
 * This is the entry point for the fts3-transfer-status command line tool.
 */
int main(int ac, char* av[]) {

	try {
		// create and initialize the command line utility
		auto_ptr<TransferStatusCli> cli (
				getCli<TransferStatusCli>(ac, av)
			);

		// validate command line options, and return respective gsoap context
		GSoapContextAdapter* ctx = cli->validate();
		if (!ctx) return 0;

		// get job IDs that have to be check
		vector<string> jobIds = cli->getJobIds();
		// iterate over job IDs
		vector<string>::iterator it;
		for (it = jobIds.begin(); it < jobIds.end(); it++) {

			//string job = "1215375b-5407-11e1-9322-bc3d2ed9263b"
			string jobId = *it;

			if (cli->isVerbose()) {

				if (ctx->isItVersion330()) {
					// if version higher than 3.3.0 use getTransferJobSummary2

					// do the request
					impltns__getTransferJobSummary2Response resp;
					ctx->getTransferJobSummary2(jobId, resp);

					// print the response
					if (resp._getTransferJobSummary2Return) {

						JobStatusHandler::printJobStatus(resp._getTransferJobSummary2Return->jobStatus);

					    cout << "\tActive: " << resp._getTransferJobSummary2Return->numActive << endl;
					    cout << "\tReady: " << resp._getTransferJobSummary2Return->numReady << endl;
					    cout << "\tCanceled: " << resp._getTransferJobSummary2Return->numCanceled << endl;
					    cout << "\tFinished: " << resp._getTransferJobSummary2Return->numFinished << endl;
					    cout << "\tSubmitted: " << resp._getTransferJobSummary2Return->numSubmitted << endl;
					    cout << "\tFailed: " << resp._getTransferJobSummary2Return->numFailed << endl;
					}

				} else {
					// if version higher than 3.3.0 use getTransferJobSummary

					// do the request
					impltns__getTransferJobSummaryResponse resp;
					ctx->getTransferJobSummary(jobId, resp);

					// print the response
					if (resp._getTransferJobSummaryReturn) {

						JobStatusHandler::printJobStatus(resp._getTransferJobSummaryReturn->jobStatus);

					    cout << "\tActive: " << resp._getTransferJobSummaryReturn->numActive << endl;
					    cout << "\tCanceled: " << resp._getTransferJobSummaryReturn->numCanceled << endl;
					    cout << "\tSubmitted: " << resp._getTransferJobSummaryReturn->numSubmitted << endl;
					    cout << "\tFailed: " << resp._getTransferJobSummaryReturn->numFailed << endl;
					}
				}
			} else {

				// do the request
				impltns__getTransferJobStatusResponse resp;
				ctx->getTransferJobStatus(jobId, resp);

		    	// print the response
		    	if (resp._getTransferJobStatusReturn) {
		    		cout << *resp._getTransferJobStatusReturn->jobStatus << endl;
		    	}
			}

			// TODO test!
			// check if the -l option has been used
			if (cli->list()) {

				// do the request
				impltns__getFileStatusResponse resp;
				ctx->getFileStatus(jobId, resp);

				if (resp._getFileStatusReturn) {

					std::vector<tns3__FileTransferStatus * >& vect = resp._getFileStatusReturn->item;
					std::vector<tns3__FileTransferStatus * >::iterator it;

					// print the response
					for (it = vect.begin(); it < vect.end(); it++) {
						tns3__FileTransferStatus* stat = *it;

						cout << "  Source:      " << *stat->sourceSURL << endl;
						cout << "  Destination: " << *stat->destSURL << endl;
						cout << "  State:       " << *stat->transferFileState << endl;;
						cout << "  Retries:     " << stat->numFailures << endl;
						cout << "  Reason:      " << *stat->reason << endl;
						cout << "  Duration:    " << stat->duration << endl;
					}
				}
			}
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
