/*
 * RequestLister.cpp
 *
 *  Created on: Mar 9, 2012
 *      Author: simonm
 */

#include "RequestLister.h"

#include "db/generic/SingleDbInstance.h"
#include "common/logger.h"
#include "GSoapExceptionHandler.h"
#include "common/JobStatusHandler.h"
#include "JobStatusCopier.h"

using namespace db;
using namespace fts3::ws;
using namespace fts3::common;

RequestLister::RequestLister(::soap* soap, fts__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates): soap(soap) {

	checkGivenStates (inGivenStates);
	DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates->item, "", "", "");
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's statuses have been read from the database" << commit;
}

RequestLister::RequestLister(::soap* soap, fts__ArrayOf_USCOREsoapenc_USCOREstring *inGivenStates, string dn, string vo): soap(soap) {

	checkGivenStates (inGivenStates);
	DBSingleton::instance().getDBObjectInstance()->listRequests(jobs, inGivenStates->item, "", dn, vo);
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's statuses have been read from the database" << commit;
}

RequestLister::~RequestLister() {

}

fts__ArrayOf_USCOREtns3_USCOREJobStatus* RequestLister::list() {

	// create the object
	fts__ArrayOf_USCOREtns3_USCOREJobStatus* result;
	result = soap_new_fts__ArrayOf_USCOREtns3_USCOREJobStatus(soap, -1);

	// fill it with job statuses
	vector<JobStatus*>::iterator it;
	for (it = jobs.begin(); it < jobs.end(); it++) {
		transfer__JobStatus* job_ptr = JobStatusCopier::copyJobStatus(soap, *it);
		result->item.push_back(job_ptr);
		delete *it;
	}
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

	return result;
}

void RequestLister::checkGivenStates(fts__ArrayOf_USCOREsoapenc_USCOREstring* inGivenStates) {

	if (!inGivenStates || inGivenStates->item.empty()) {
		transfer__InvalidArgumentException* ex = GSoapExceptionHandler::createInvalidArgumentException(soap, "No states were defined");
		throw ex;
	}

	JobStatusHandler& handler = JobStatusHandler::getInstance();
	vector<string>::iterator it;
	for (it = inGivenStates->item.begin(); it < inGivenStates->item.end(); it++) {
		if(handler.isStateValid(*it)) {
			transfer__InvalidArgumentException* ex = GSoapExceptionHandler::createInvalidArgumentException(soap,
					"Unknown job status: " + *it);
			throw ex;
		}
	}
}
