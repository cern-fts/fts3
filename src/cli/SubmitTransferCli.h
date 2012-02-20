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

/*
 * SubmitTransferCli.h
 *
 *  Created on: Feb 2, 2012
 *      Author: simonm
 */

#ifndef SUBMITTRANSFERCLI_H_
#define SUBMITTRANSFERCLI_H_

#include "CliBase.h"
#include "GsoapStubs.h"
#include <vector>

using namespace std;


namespace fts { namespace cli {

class SubmitTransferCli : public CliBase {

	struct Task {
		string src;
		string dest;
		string checksum;
	};

public:
	SubmitTransferCli();
	virtual ~SubmitTransferCli();


	virtual void initCli(int ac, char* av[]);
	bool createJobElements();
	bool performChecks();

	string getUsageString();
	string getPassword();
	string getSource();
	string getDestination();
	transfer__TransferParams* getParams(soap* soap);
	vector<transfer__TransferJobElement * > getJobElements(soap* soap);
	vector<transfer__TransferJobElement2 * > getJobElements2(soap* soap);


	bool useDelegation() {
		return delegate;
	}

	bool useCheckSum() {
		return checksum;
	}

	bool isBlocking();

private:
	options_description specific;
	options_description hidden;
	string cfg_file;
	string password;

	vector<Task> tasks;

	bool checksum;
	bool delegate;

	// parameters names
	string FTS3_PARAM_GRIDFTP;
	string FTS3_PARAM_MYPROXY;
	string FTS3_PARAM_DELEGATIONID;
	string FTS3_PARAM_SPACETOKEN;
	string FTS3_PARAM_SPACETOKEN_SOURCE;
	string FTS3_PARAM_COPY_PIN_LIFETIME;
	string FTS3_PARAM_LAN_CONNECTION;
	string FTS3_PARAM_FAIL_NEARLINE;
	string FTS3_PARAM_OVERWRITEFLAG;
	string FTS3_PARAM_CHECKSUM_METHOD;
};

}
}

#endif /* SUBMITTRANSFERCLI_H_ */
