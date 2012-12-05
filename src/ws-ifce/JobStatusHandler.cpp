/*
 * JobStatusHandler.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#include "JobStatusHandler.h"
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

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

JobStatusHandler::JobStatusHandler():
		statusNameToId(map_list_of
			(FTS3_STATUS_FINISHEDDIRTY, FTS3_STATUS_FINISHEDDIRTY_ID)
			(FTS3_STATUS_CANCELED, FTS3_STATUS_CANCELED_ID)
			(FTS3_STATUS_UNKNOWN, FTS3_STATUS_UNKNOWN_ID)
			(FTS3_STATUS_SUBMITTED, FTS3_STATUS_SUBMITTED_ID)
			(FTS3_STATUS_ACTIVE, FTS3_STATUS_ACTIVE_ID)
			(FTS3_STATUS_FINISHED, FTS3_STATUS_FINISHED_ID)
			(FTS3_STATUS_READY, FTS3_STATUS_READY_ID)
			(FTS3_STATUS_FAILED, FTS3_STATUS_FAILED_ID).to_container(statusNameToId)) {

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

bool JobStatusHandler::isStatusValid(string status) {

	to_upper(status);
	return statusNameToId.find(status) != statusNameToId.end();
}

int JobStatusHandler::countInState(const string status, vector<JobStatus*>& statuses) {

	int count = 0;

	vector<JobStatus*>::iterator it;
	for (it = statuses.begin(); it < statuses.end(); it++) {
		if (status == (*it)->fileStatus) {
			++count;
		}
	}

	return count;
}
