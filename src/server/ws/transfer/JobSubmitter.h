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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
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

#include "ws-ifce/gsoap/gsoap_stubs.h"
#include "db/generic/GenericDbIfce.h"
#include "common/JobParameterHandler.h"

#include <string>
#include <map>
#include <vector>
#include <list>
#include <utility>

#include <boost/regex.hpp>
#include <boost/tuple/tuple.hpp>

namespace fts3 { namespace ws {

using namespace std;
using namespace fts3::common;
using namespace boost;

/**
 * The JobSubmitter class takes care of submitting transfers
 *
 * Depending on the request (submitTransfer, submitTransfer2, submitTransfer3)
 * different constructors should be used
 *
 */
class JobSubmitter {

	enum {
		SHARE,
		CONTENT
	};

	enum {
		SOURCE = 0,
		DESTINATION,
		VO
	};

	typedef tuple<string, string, string> share;
	typedef pair<bool, bool> content;
	typedef tuple< share, content > cfg_type;

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
	 * Constructor - creates a submitter object that should be used
	 * by submitTransfer4 requests
	 *
	 * @param soap - the soap object that is serving the given request
	 * @param job - the job that has to be submitted
	 */
	JobSubmitter(soap* ctx, tns3__TransferJob3 *job);

	/**
	 * Destructor
	 */
	virtual ~JobSubmitter();

	/**
	 * submits the job
	 */
	string submit();

private:

	/// DB instance
	GenericDbIfce* db;

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
	/// delegation ID
	string delegationId;
	/// used only for compatibility reason
	string sourceSpaceTokenDescription;
	/// user password
	string cred;
	/// copy lifetime pin
	int copyPinLifeTime;
	/// source SE (it's the same for all files in the transfer job)
	string sourceSe;
	/// destination SE (it's the same for all files in the transfer job)
	string destinationSe;

	/// maps job parameter values to their names
	JobParameterHandler params;

	/**
	 * the job elements that have to be submitted (each job is a tuple of source,
	 * destination, and optionally checksum)
	 */
	vector<job_element_tupple> jobs;

	/**
	 * The common initialization for both parameterized constructors
	 *
	 * @param jobParams - job parameters
	 */
	void init(tns3__TransferParams *jobParams);

	/**
	 * Checks:
	 * - if the SE has been blacklisted (if yes an exception is thrown)
	 * - if the SE is in BDII and the state is either 'production' or 'online'
	 *   (if it is in BDII but the state is wrong an exception is thrown)
	 * - if the SE is in BDII and the submitted VO is on the VOsAllowed list
	 *   (if it is in BDII but the VO is not on the list an exception is thrown)
	 * - if the SE is in the OSG and if is's active and not disabled
	 *   (it it is in BDII but the conditions are not met an exception is thrown)
	 */
	void checkSe(string se);

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

	map<string, string> pairSourceAndDestination(vector<string> sources, vector<string> destinations);

	///
	static const regex fileUrlRegex;

	static const string false_str;

	/**
	 *
	 */
	string fileUrlToSeName(string url);

};

}
}

#endif /* JOBSUBMITTER_H_ */
