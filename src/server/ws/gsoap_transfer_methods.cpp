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
 */

#include "ws-ifce/gsoap/gsoap_stubs.h"
#include "uuid_generator.h"

#include "db/generic/SingleDbInstance.h"
#include "config/serverconfig.h"

#include "ws/JobSubmitter.h"
#include "ws/RequestLister.h"
#include "ws/AuthorizationManager.h"

#include "common/JobStatusHandler.h"
#include "common/logger.h"
#include "common/error.h"


using namespace boost;
using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace fts3::common;
using namespace std;

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit(soap *soap, tns3__TransferJob *_job, struct impltns__transferSubmitResponse &_param_3) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		JobSubmitter submitter (soap, _job, false);
		_param_3._transferSubmitReturn = submitter.submit();

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit2(soap *soap, tns3__TransferJob *_job, struct impltns__transferSubmit2Response &_param_4) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		JobSubmitter submitter (soap, _job, true);
		_param_4._transferSubmit2Return = submitter.submit();

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit3(soap *soap, tns3__TransferJob2 *_job, struct impltns__transferSubmit3Response &_param_5) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		JobSubmitter submitter (soap, _job);
		_param_5._transferSubmit3Return = submitter.submit();

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int fts3::impltns__listRequests(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct impltns__listRequestsResponse &_param_7) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		RequestLister lister(soap, _inGivenStates);
		_param_7._listRequestsReturn = lister.list();

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int fts3::impltns__listRequests2(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string _forDN, string _forVO, struct impltns__listRequests2Response &_param_8) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests2' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		RequestLister lister(soap, _inGivenStates);
		_param_8._listRequests2Return = lister.list();

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}


/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus(soap *soap, string _requestID, int _offset, int _limit, struct impltns__getFileStatusResponse &_param_9) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getFileStatus' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		vector<FileTransferStatus*> statuses;
		vector<FileTransferStatus*>::iterator it;

		// create response
		_param_9._getFileStatusReturn = soap_new_impltns__ArrayOf_USCOREtns3_USCOREFileTransferStatus(soap, -1);
		DBSingleton::instance().getDBObjectInstance()->getTransferFileStatus(_requestID, statuses);

		for (it = statuses.begin() + _offset; it < statuses.end() && it < statuses.begin() + _offset + _limit; it++) {

			FileTransferStatus* tmp = *it;

			tns3__FileTransferStatus* status = soap_new_tns3__FileTransferStatus(soap, -1);

			status->destSURL = soap_new_std__string(soap, -1);
			*status->destSURL = tmp->destSURL;

			status->logicalName = soap_new_std__string(soap, -1);
			*status->logicalName = tmp->logicalName;

			status->reason = soap_new_std__string(soap, -1);
			*status->reason = tmp->reason;

			status->reason_USCOREclass = soap_new_std__string(soap, -1);
			*status->reason_USCOREclass = tmp->reason_class;

			status->sourceSURL = soap_new_std__string(soap, -1);
			*status->sourceSURL = tmp->sourceSURL;

			status->transferFileState = soap_new_std__string(soap, -1);
			*status->transferFileState = tmp->transferFileState;

			status->duration = tmp->finish_time - tmp->start_time;
			status->numFailures = 0;//tmp->numFailures; //TODO retries not implemented yet

			_param_9._getFileStatusReturn->item.push_back(status);
			delete tmp;
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus2(soap *soap, string _requestID, int _offset, int _limit, struct impltns__getFileStatus2Response &_param_10) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getFileStatus2' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		vector<FileTransferStatus*> statuses;
		vector<FileTransferStatus*>::iterator it;

		// create response
		_param_10._getFileStatus2Return = soap_new_impltns__ArrayOf_USCOREtns3_USCOREFileTransferStatus2(soap, -1);
		DBSingleton::instance().getDBObjectInstance()->getTransferFileStatus(_requestID, statuses);

		for (it = statuses.begin() + _offset; it < statuses.end() && it < statuses.begin() + _offset + _limit; it++) {

			FileTransferStatus* tmp = *it;

			tns3__FileTransferStatus2* status = soap_new_tns3__FileTransferStatus2(soap, -1);

			status->destSURL = soap_new_std__string(soap, -1);
			*status->destSURL = tmp->destSURL;

			status->logicalName = soap_new_std__string(soap, -1);
			*status->logicalName = tmp->logicalName;

			status->reason = soap_new_std__string(soap, -1);
			*status->reason = tmp->reason;

			status->reason_USCOREclass = soap_new_std__string(soap, -1);
			*status->reason_USCOREclass = tmp->reason_class;

			status->sourceSURL = soap_new_std__string(soap, -1);
			*status->sourceSURL = tmp->sourceSURL;

			status->transferFileState = soap_new_std__string(soap, -1);
			*status->transferFileState = tmp->transferFileState;

			status->duration = tmp->finish_time - tmp->start_time;
			status->numFailures = tmp->numFailures;

			_param_10._getFileStatus2Return->item.push_back(status);
			delete tmp;
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobStatus(soap *soap, string _requestID, struct impltns__getTransferJobStatusResponse &_param_11) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getTransferJobStatus' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap);
		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(!fileStatuses.empty()){
			_param_11._getTransferJobStatusReturn = JobStatusHandler::getInstance().copyJobStatus(
					soap, *fileStatuses.begin()
				);
			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

			vector<JobStatus*>::iterator it;
			for (it = fileStatuses.begin(); it < fileStatuses.end(); it++) {
				delete *it;
			}
		} else {
			throw Err_Custom("requestID <" + _requestID + "> was not found");
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobSummary(soap *soap, string _requestID, struct impltns__getTransferJobSummaryResponse &_param_12) {

	try {
		AuthorizationManager::getInstance().authorize(soap);
		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if (!fileStatuses.empty()) {

			_param_12._getTransferJobSummaryReturn = soap_new_tns3__TransferJobSummary(soap, -1);
			_param_12._getTransferJobSummaryReturn->jobStatus = JobStatusHandler::getInstance().copyJobStatus(
					soap, *fileStatuses.begin()
				);

			JobStatusHandler& handler = JobStatusHandler::getInstance();
			_param_12._getTransferJobSummaryReturn->numActive = handler.countInState(
					JobStatusHandler::FTS3_STATUS_ACTIVE,
					fileStatuses
				);
			_param_12._getTransferJobSummaryReturn->numCanceled = handler.countInState(
					JobStatusHandler::FTS3_STATUS_CANCELED,
					fileStatuses
				);
			_param_12._getTransferJobSummaryReturn->numSubmitted = handler.countInState(
					JobStatusHandler::FTS3_STATUS_SUBMITTED,
					fileStatuses
				);
			_param_12._getTransferJobSummaryReturn->numFinished = handler.countInState(
					JobStatusHandler::FTS3_STATUS_FINISHED,
					fileStatuses
				);
			_param_12._getTransferJobSummaryReturn->numFailed = handler.countInState(
					JobStatusHandler::FTS3_STATUS_FAILED,
					fileStatuses
				);

			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

			vector<JobStatus*>::iterator it;
			for (it = fileStatuses.begin(); it < fileStatuses.end(); it++) {
				delete *it;
			}

		} else {
			throw Err_Custom("requestID <" + _requestID + "> was not found");
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary2' (returns error code or SOAP_OK)
int fts3::impltns__getTransferJobSummary2(soap *soap, string _requestID, struct impltns__getTransferJobSummary2Response &_param_13) {

	try {
		AuthorizationManager::getInstance().authorize(soap);
		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(!fileStatuses.empty()) {

			_param_13._getTransferJobSummary2Return = soap_new_tns3__TransferJobSummary2(soap, -1);
			_param_13._getTransferJobSummary2Return->jobStatus = JobStatusHandler::getInstance().copyJobStatus(
					soap, *fileStatuses.begin()
				);

			JobStatusHandler& handler = JobStatusHandler::getInstance();
			_param_13._getTransferJobSummary2Return->numActive = handler.countInState(
					JobStatusHandler::FTS3_STATUS_ACTIVE,
					fileStatuses
				);
			_param_13._getTransferJobSummary2Return->numCanceled = handler.countInState(
					JobStatusHandler::FTS3_STATUS_CANCELED,
					fileStatuses
				);
			_param_13._getTransferJobSummary2Return->numSubmitted = handler.countInState(
					JobStatusHandler::FTS3_STATUS_SUBMITTED,
					fileStatuses
				);
			_param_13._getTransferJobSummary2Return->numFinished = handler.countInState(
					JobStatusHandler::FTS3_STATUS_FINISHED,
					fileStatuses
				);
			_param_13._getTransferJobSummary2Return->numReady = handler.countInState(
					JobStatusHandler::FTS3_STATUS_READY,
					fileStatuses
				);
			_param_13._getTransferJobSummary2Return->numFailed = handler.countInState(
					JobStatusHandler::FTS3_STATUS_FAILED,
					fileStatuses
				);

			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

			vector<JobStatus*>::iterator it;
			for (it = fileStatuses.begin(); it < fileStatuses.end(); it++) {
				delete *it;
			}

		} else {
			throw Err_Custom("requestID <" + _requestID + "> was not found");
		}

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}



/// Web service operation 'getVersion' (returns error code or SOAP_OK)
int fts3::impltns__getVersion(soap *soap, struct impltns__getVersionResponse &_param_21) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getVersion' request" << commit;
	_param_21.getVersionReturn = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int fts3::impltns__getSchemaVersion(soap *soap, struct impltns__getSchemaVersionResponse &_param_22) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getSchemaVersion' request" << commit;
	_param_22.getSchemaVersionReturn = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int fts3::impltns__getInterfaceVersion(soap *soap, struct impltns__getInterfaceVersionResponse &_param_23) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getInterfaceVersion' request" << commit;
	_param_23.getInterfaceVersionReturn = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int fts3::impltns__getServiceMetadata(soap *soap, string _key, struct impltns__getServiceMetadataResponse &_param_24) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getServiceMetadata' request" << commit;
	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	return SOAP_OK;
}


/// Web service operation 'cancel' (returns error code or SOAP_OK)
int fts3::impltns__cancel(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct impltns__cancelResponse &_param_14) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'cancel' request" << commit;

	try{
		AuthorizationManager::getInstance().authorize(soap);

		if (_requestIDs) {
			vector<string> &jobs = _requestIDs->item;
			std::vector<std::string>::iterator jobsIter;
			if (!jobs.empty()) {
				 std::string jobId("");
				 for (jobsIter = jobs.begin(); jobsIter != jobs.end(); ++jobsIter)
					jobId += *jobsIter;

				FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Jobs that will be canceled:" << jobId << commit;
				DBSingleton::instance().getDBObjectInstance()->cancelJob(jobs);
			}
		}
	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");

		return SOAP_FAULT;
	} catch (...) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, job can't be canceled "  << commit;
	    return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'setJobPriority' (returns error code or SOAP_OK)
int fts3::impltns__setJobPriority(soap *soap, string _requestID, int _priority, struct impltns__setJobPriorityResponse &_param_15) {
	return SOAP_OK;
}

/// Web service operation 'addVOManager' (returns error code or SOAP_OK)
int fts3::impltns__addVOManager(soap *soap, string _VOName, string _principal, struct impltns__addVOManagerResponse &_param_16) {
	return SOAP_OK;
}

/// Web service operation 'removeVOManager' (returns error code or SOAP_OK)
int fts3::impltns__removeVOManager(soap *soap, string _VOName, string _principal, struct impltns__removeVOManagerResponse &_param_17) {
	return SOAP_OK;
}

/// Web service operation 'listVOManagers' (returns error code or SOAP_OK)
int fts3::impltns__listVOManagers(soap *soap, string _VOName, struct impltns__listVOManagersResponse &_param_18) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listVOManagers' request" << commit;

	_param_18._listVOManagersReturn = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(soap, -1);
	_param_18._listVOManagersReturn->item.push_back("default username");

	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int fts3::impltns__getRoles(soap *soap, struct impltns__getRolesResponse &_param_19) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRoles' request" << commit;

	_param_19.getRolesReturn = soap_new_tns3__Roles(soap, -1);

	_param_19.getRolesReturn->clientDN = soap_new_std__string(soap, -1);
	*_param_19.getRolesReturn->clientDN = "clientDN";
	_param_19.getRolesReturn->serviceAdmin = soap_new_std__string(soap, -1);
	*_param_19.getRolesReturn->serviceAdmin = "serviceAdmin";
	_param_19.getRolesReturn->submitter = soap_new_std__string(soap, -1);
	*_param_19.getRolesReturn->submitter = "submitter";

	tns3__StringPair* pair = soap_new_tns3__StringPair(soap, -1);
	pair->string1 = soap_new_std__string(soap, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(soap, -1);
	*pair->string2 = "string2";
	_param_19.getRolesReturn->VOManager.push_back(pair);

	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int fts3::impltns__getRolesOf(soap *soap, string _otherDN, struct impltns__getRolesOfResponse &_param_20) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRolesOf' request" << commit;

	_param_20._getRolesOfReturn = soap_new_tns3__Roles(soap, -1);

	_param_20._getRolesOfReturn->clientDN = soap_new_std__string(soap, -1);
	*_param_20._getRolesOfReturn->clientDN = "clientDN";
	_param_20._getRolesOfReturn->serviceAdmin = soap_new_std__string(soap, -1);
	*_param_20._getRolesOfReturn->serviceAdmin = "serviceAdmin";
	_param_20._getRolesOfReturn->submitter = soap_new_std__string(soap, -1);
	*_param_20._getRolesOfReturn->submitter = "submitter";

	tns3__StringPair* pair = soap_new_tns3__StringPair(soap, -1);
	pair->string1 = soap_new_std__string(soap, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(soap, -1);
	*pair->string2 = "string2";
	_param_20._getRolesOfReturn->VOManager.push_back(pair);

	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int fts3::impltns__debugSet(struct soap*, string _source, string _destination, bool _debug, struct impltns__debugSetResponse &_param_16) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'debugSet' request" << commit;

	return SOAP_OK;
}
