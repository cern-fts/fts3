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
const string JobStatusHandler::FTS3_STATUS_FAILED = "FAILED";
const string JobStatusHandler::FTS3_STATUS_CATALOGFAILED = "CATALOGFAILED";
const string JobStatusHandler::FTS3_STATUS_FINISHED_DIRTY = "FINISHEDDIRTY";
const string JobStatusHandler::FTS3_STATUS_CANCELED = "CANCELED";
const string JobStatusHandler::FTS3_STATUS_TRANSFERFAILED = "FAILED";
const string JobStatusHandler::FTS3_STATUS_FINISHED = "FINISHED";
const string JobStatusHandler::FTS3_STATUS_SUBMITTED = "SUBMITTED";
const string JobStatusHandler::FTS3_STATUS_PENDING = "PENDING";
const string JobStatusHandler::FTS3_STATUS_ACTIVE = "ACTIVE";
const string JobStatusHandler::FTS3_STATUS_CANCELING = "CANCELING";
const string JobStatusHandler::FTS3_STATUS_WAITING = "WAITING";
const string JobStatusHandler::FTS3_STATUS_HOLD = "HOLD";
const string JobStatusHandler::FTS3_STATUS_DONE = "DONE";
const string JobStatusHandler::FTS3_STATUS_READY = "READY";
const string JobStatusHandler::FTS3_STATUS_DONEWITHERRORS = "DONEWITHERRORS";
const string JobStatusHandler::FTS3_STATUS_FINISHING = "FINISHING";
const string JobStatusHandler::FTS3_STATUS_AWAITING_PRESTAGE = "AWAITINGPRESTAGE";
const string JobStatusHandler::FTS3_STATUS_PRESTAGING = "PRESTAGING";
const string JobStatusHandler::FTS3_STATUS_WAITING_PRESTAGE = "WAITINGPRESTAGE";
const string JobStatusHandler::FTS3_STATUS_WAITING_CATALOG_RESOLUTION = "WAITINGCATALOGRESOLUTION";
const string JobStatusHandler::FTS3_STATUS_WAITING_CATALOG_REGISTRATION = "WAITINGCATALOGREGISTRATION";

JobStatusHandler::JobStatusHandler(): statuses(map_list_of
		(FTS3_STATUS_SUBMITTED, FTS3_STATUS_SUBMITTED_ID)
		(FTS3_STATUS_PENDING, FTS3_STATUS_PENDING_ID)
		(FTS3_STATUS_ACTIVE, FTS3_STATUS_ACTIVE_ID)
		(FTS3_STATUS_CANCELING, FTS3_STATUS_CANCELING_ID)
		(FTS3_STATUS_DONE, FTS3_STATUS_DONE_ID)
		(FTS3_STATUS_FINISHED, FTS3_STATUS_FINISHED_ID)
		(FTS3_STATUS_FINISHED_DIRTY, FTS3_STATUS_FINISHED_DIRTY_ID)
		(FTS3_STATUS_CANCELED, FTS3_STATUS_CANCELED_ID)
		(FTS3_STATUS_WAITING, FTS3_STATUS_WAITING_ID)
		(FTS3_STATUS_HOLD, FTS3_STATUS_HOLD_ID)
		(FTS3_STATUS_TRANSFERFAILED, FTS3_STATUS_TRANSFERFAILED_ID)
		(FTS3_STATUS_CATALOGFAILED, FTS3_STATUS_CATALOGFAILED_ID)
		(FTS3_STATUS_READY, FTS3_STATUS_READY_ID)
		(FTS3_STATUS_DONEWITHERRORS, FTS3_STATUS_DONEWITHERRORS_ID)
		(FTS3_STATUS_FINISHING, FTS3_STATUS_FINISHING_ID)
		(FTS3_STATUS_FAILED, FTS3_STATUS_FAILED_ID)
		(FTS3_STATUS_AWAITING_PRESTAGE, FTS3_STATUS_AWAITING_PRESTAGE_ID)
		(FTS3_STATUS_PRESTAGING, FTS3_STATUS_PRESTAGING_ID)
		(FTS3_STATUS_WAITING_PRESTAGE, FTS3_STATUS_WAITING_PRESTAGE_ID)
		(FTS3_STATUS_WAITING_CATALOG_RESOLUTION,FTS3_STATUS_WAITING_CATALOG_RESOLUTION_ID)
		(FTS3_STATUS_WAITING_CATALOG_REGISTRATION, FTS3_STATUS_WAITING_CATALOG_REGISTRATION_ID).to_container(statuses)) {

	// the constant map is being initialized in initializer list
}

bool JobStatusHandler::isTransferReady(string status) {

	to_upper(status);
	map<string, JobStateEnum>::const_iterator it = statuses.find(status);

	if (it == statuses.end()) {
		return FTS3_STATUS_UNKNOWN_ID <= 0;
	}
	return it->second <= 0;
}

bool JobStatusHandler::isStatusValid(string status) {

	to_upper(status);
	return statuses.find(status) != statuses.end();
}
