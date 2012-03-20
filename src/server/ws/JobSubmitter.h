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
 * JobSubmitter.h
 *
 *  Created on: Mar 7, 2012
 *      Author: Michal Simon
 */

#ifndef JOBSUBMITTER_H_
#define JOBSUBMITTER_H_

#include "gsoap_transfer_stubs.h"
#include "db/generic/GenericDbIfce.h"
#include "common/JobParameterHandler.h"

#include <string>
#include <map>
#include <vector>

using namespace std;
using namespace fts3::common;

namespace fts3 { namespace ws {

/**
 * The JobSubmitter class takes care of submitting transfers
 *
 * Depending on the request (submitTransfer, submitTransfer2, submitTransfer3)
 * different constructors should be used
 *
 */
class JobSubmitter {

public:
	/**
	 * Constructor - creates a submitter object that should be used
	 * by submitTransfer and submitTransfer2 requests
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param job - the job that has to be submitted
	 * @param delegation - should be true if delegation is being used
	 */
	JobSubmitter(soap* soap, tns3__TransferJob *job, bool delegation);

	/**
	 * Constructor - creates a submitter object that should be used
	 * by submitTransfer3 requests
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param job - the job that has to be submitted
	 */
	JobSubmitter(soap* soap, tns3__TransferJob2 *job);

	/**
	 * Destructor
	 */
	virtual ~JobSubmitter();

	/**
	 * submits the job
	 */
	string submit();

private:

	/**
	 * Default constructor.
	 *
	 * Private, should not be used.
	 */
	JobSubmitter() {};

	/// job ID
	string id;
	/// user DN
	string dn;
	/// user VO
	string vo;
	/// used only for compatibility reason
	string sourceSpaceTokenDescription;
	/// user password
	string cred;
	/// copy lifetime pin
	int copyPinLifeTime;

	/// maps job parameter values to their names
	JobParameterHandler params;

	/**
	 * the job elements that have to be submitted (each job is a tuple of source,
	 * destination, and optionally checksum)
	 */
	vector<src_dest_checksum_tupple> jobs;

	/**
	 * The common initialization for both parameterized constructors
	 *
	 * @param jobParams - job parameters
	 */
	void init(tns3__TransferParams *jobParams);

	/**
	 * Checks whether the right protocol has been used
	 *
	 * @file - source or destination file
	 * @return true if right protol has been used
	 */
	bool checkProtocol(string file);

	/**
	 * Checks if the file is given as a logical file name
	 *
	 * @param - file name
	 * @return true if its a logical file name
	 */
	bool checkIfLfn(string file);
};

}
}

#endif /* JOBSUBMITTER_H_ */
