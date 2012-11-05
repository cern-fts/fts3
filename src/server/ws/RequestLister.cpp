/*
 * RequestLister.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#include "RequestLister.h"

#include "db/generic/SingleDbInstance.h"

#include "CGsiAdapter.h"

#include "common/error.h"
#include "common/logger.h"
#include "common/JobStatusHandler.h"

using namespace db;
using namespace fts3::ws;
using namespace fts3::common;

RequestLister::RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates): soap(soap) {

	CGsiAdapter cgsi(soap);
	string dn = cgsi.getClientDn();
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is listing transfer job requests" << commit;

	checkGivenStates (inGivenStates);
	DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates->item, "", "", "");
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's statuses have been read from the database" << commit;
}

RequestLister::RequestLister(::soap* soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates, string dn, string vo): soap(soap) {

	CGsiAdapter cgsi(soap);
	string user = cgsi.getClientDn();
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << user << " is listing transfer job requests" << commit;

	checkGivenStates (inGivenStates);
	DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates->item, "", dn, vo);
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's statuses have been read from the database" << commit;
}

RequestLister::~RequestLister() {

}

impltns__ArrayOf_USCOREtns3_USCOREJobStatus* RequestLister::list() {

	// create the object
	impltns__ArrayOf_USCOREtns3_USCOREJobStatus* result;
	result = soap_new_impltns__ArrayOf_USCOREtns3_USCOREJobStatus(soap, -1);

	// fill it with job statuses
	vector<JobStatus*>::iterator it;
	for (it = jobs.begin(); it < jobs.end(); it++) {
		tns3__JobStatus* job_ptr = JobStatusHandler::getInstance().copyJobStatus(soap, *it);
		result->item.push_back(job_ptr);
		delete *it;
	}
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

	return result;
}

void RequestLister::checkGivenStates(impltns__ArrayOf_USCOREsoapenc_USCOREstring* inGivenStates) {

	if (!inGivenStates || inGivenStates->item.empty()) {
		throw Err_Custom("No states were defined!");
	}

	JobStatusHandler& handler = JobStatusHandler::getInstance();
	vector<string>::iterator it;
	for (it = inGivenStates->item.begin(); it < inGivenStates->item.end(); it++) {
		if(!handler.isStatusValid(*it)) {
			throw Err_Custom("Unknown job status: " + *it);
		}
	}
}
