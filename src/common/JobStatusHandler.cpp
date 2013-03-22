/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
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
 * JobStatusHandler.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: Michał Simon
 */

#include "JobStatusHandler.h"
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

#include <set>

using namespace boost::assign;
using namespace fts3::common;
using namespace boost;

// initialize string constants
const string JobStatusHandler::FTS3_STATUS_FINISHEDDIRTY = "FINISHEDDIRTY";
const string JobStatusHandler::FTS3_STATUS_CANCELED = "CANCELED";
const string JobStatusHandler::FTS3_STATUS_UNKNOWN = "UNKNOWN";
const string JobStatusHandler::FTS3_STATUS_FAILED = "FAILED";
const string JobStatusHandler::FTS3_STATUS_FINISHED = "FINISHED";
const string JobStatusHandler::FTS3_STATUS_SUBMITTED = "SUBMITTED";
const string JobStatusHandler::FTS3_STATUS_READY = "READY";
const string JobStatusHandler::FTS3_STATUS_ACTIVE = "ACTIVE";
const string JobStatusHandler::FTS3_STATUS_STAGING = "STAGING";
const string JobStatusHandler::FTS3_STATUS_NOT_USED = "NOT_USED";

JobStatusHandler::JobStatusHandler():
		statusNameToId(map_list_of
			(FTS3_STATUS_FINISHEDDIRTY, FTS3_STATUS_FINISHEDDIRTY_ID)
			(FTS3_STATUS_CANCELED, FTS3_STATUS_CANCELED_ID)
			(FTS3_STATUS_UNKNOWN, FTS3_STATUS_UNKNOWN_ID)
			(FTS3_STATUS_SUBMITTED, FTS3_STATUS_SUBMITTED_ID)
			(FTS3_STATUS_ACTIVE, FTS3_STATUS_ACTIVE_ID)
			(FTS3_STATUS_FINISHED, FTS3_STATUS_FINISHED_ID)
			(FTS3_STATUS_READY, FTS3_STATUS_READY_ID)
			(FTS3_STATUS_FAILED, FTS3_STATUS_FAILED_ID)
			(FTS3_STATUS_STAGING, FTS3_STATUS_STAGING_ID)
			(FTS3_STATUS_NOT_USED, FTS3_STATUS_NOT_USED_ID).to_container(statusNameToId)){

	// the constant map is initialized in initializer list
}

bool JobStatusHandler::isTransferFinished(string status) {

	to_upper(status);
	map<string, JobStatusEnum>::const_iterator it = statusNameToId.find(status);

	if (it == statusNameToId.end()) {
		return FTS3_STATUS_UNKNOWN_ID <= 0;
	}
	return it->second <= 0;
}

bool JobStatusHandler::isStatusValid(string& status) {

	to_upper(status);
	return statusNameToId.find(status) != statusNameToId.end();
}

size_t JobStatusHandler::countInState(const string status, vector<JobStatus*>& statuses) {

	set<int> files;

	vector<JobStatus*>::iterator it;
	for (it = statuses.begin(); it < statuses.end(); it++) {
		if (status == (*it)->fileStatus) {
			files.insert((*it)->fileIndex);
		}
	}

	return files.size();
}
