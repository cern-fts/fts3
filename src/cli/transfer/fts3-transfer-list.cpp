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
 * fts3-transfer-list.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: Michal Simon
 */


#include "GSoapContextAdapter.h"
#include "ui/ListTransferCli.h"

#include "common/JobStatusHandler.h"

#include <memory>

using namespace std;
using namespace fts3::cli;
using namespace fts3::common;


/**
 * This is the entry point for the fts3-transfer-list command line tool.
 */
int main(int ac, char* av[]) {

	try {
		// create and initialize the command line utility
		auto_ptr<ListTransferCli> cli (
				getCli<ListTransferCli>(ac, av)
			);

		// validate command line options, and return respective gsoap context
		GSoapContextAdapter* ctx = cli->validate();
		if (!ctx) return 0;

		if (!cli->checkIfFeaturesSupported()) {
			return 0;
		}

		impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = cli->getStatusArray();

		vector<tns3__JobStatus * > statuses;

		if (ctx->isUserVoRestrictListingSupported()) {

			impltns__listRequests2Response resp;
			ctx->listRequests2(array, cli->getUserDn(), cli->getVoName(), resp);

			statuses = resp._listRequests2Return->item;

		} else {

			impltns__listRequestsResponse resp;
			ctx->listRequests(array, resp);

			statuses = resp._listRequestsReturn->item;
		}

		vector<tns3__JobStatus * >::iterator it;
		for (it = statuses.begin(); it < statuses.end(); it++) {
			if (cli->isVerbose()) {

				JobStatusHandler::printJobStatus(*it);

			} else {
				cout << *(*it)->jobID << "\t" << *(*it)->jobStatus << endl;
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
