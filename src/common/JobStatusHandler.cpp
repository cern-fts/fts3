/*
 * JobStatusHandler.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#include "JobStatusHandler.h"
#include <boost/assign.hpp>

using namespace boost::assign;
using namespace fts3::common;

// initialize single instance pointer with 0
scoped_ptr<JobStatusHandler> JobStatusHandler::instance;

// initialize string constants
const string JobStatusHandler::FTS3_STATUS_FAILED = "Failed";
const string JobStatusHandler::FTS3_STATUS_CATALOGFAILED = "CatalogFailed";
const string JobStatusHandler::FTS3_STATUS_FINISHED_DIRTY = "FinishedDirty";
const string JobStatusHandler::FTS3_STATUS_CANCELED = "Canceled";
const string JobStatusHandler::FTS3_STATUS_TRANSFERFAILED = "Failed";
const string JobStatusHandler::FTS3_STATUS_FINISHED = "Finished";
const string JobStatusHandler::FTS3_STATUS_SUBMITTED = "Submitted";
const string JobStatusHandler::FTS3_STATUS_PENDING = "Pending";
const string JobStatusHandler::FTS3_STATUS_ACTIVE = "Active";
const string JobStatusHandler::FTS3_STATUS_CANCELING = "Canceling";
const string JobStatusHandler::FTS3_STATUS_WAITING = "Waiting";
const string JobStatusHandler::FTS3_STATUS_HOLD = "Hold";
const string JobStatusHandler::FTS3_STATUS_DONE = "Done";
const string JobStatusHandler::FTS3_STATUS_READY = "Ready";
const string JobStatusHandler::FTS3_STATUS_DONEWITHERRORS = "DoneWithErrors";
const string JobStatusHandler::FTS3_STATUS_FINISHING = "Finishing";
const string JobStatusHandler::FTS3_STATUS_AWAITING_PRESTAGE = "AwaitingPrestage";
const string JobStatusHandler::FTS3_STATUS_PRESTAGING = "Prestaging";
const string JobStatusHandler::FTS3_STATUS_WAITING_PRESTAGE = "WaitingPrestage";
const string JobStatusHandler::FTS3_STATUS_WAITING_CATALOG_RESOLUTION = "WaitingCatalogResolution";
const string JobStatusHandler::FTS3_STATUS_WAITING_CATALOG_REGISTRATION = "WaitingCatalogRegistration";

JobStatusHandler& JobStatusHandler::getInstance() {
	// thread safe lazy loading
    if (instance.get() == 0) {
        FTS3_COMMON_MONITOR_START_STATIC_CRITICAL
        if (instance.get() == 0) {
            instance.reset(new JobStatusHandler);
        }
        FTS3_COMMON_MONITOR_END_CRITICAL
    }
    return *instance;
}

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

JobStatusHandler::~JobStatusHandler() {

}

bool JobStatusHandler::isTransferReady(string status) {

	map<string, JobStateEnum>::const_iterator it = statuses.find(status);

	if (it == statuses.end()) {
		return FTS3_STATUS_UNKNOWN_ID <= 0;
	}
	return it->second <= 0;
}

bool JobStatusHandler::isStateValid(string state) {

	return statuses.find(state) != statuses.end();
}
