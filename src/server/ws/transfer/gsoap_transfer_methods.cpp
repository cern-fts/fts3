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

#include "JobSubmitter.h"
#include "RequestLister.h"
#include "VersionResolver.h"
#include "GSoapJobStatus.h"

//#include "ws/GSoapDelegationHandler.h"
#include "ws/CGsiAdapter.h"
#include "ws/AuthorizationManager.h"
//#include "ws/InternalLogRetriever.h"

#include "common/JobStatusHandler.h"
#include "common/TarArchiver.h"
#include "common/ZipCompressor.h"

#include "common/logger.h"
#include "common/error.h"

#include <fstream>
#include <sstream>
#include <list>

#include <boost/scoped_ptr.hpp>

#include <string.h>

#include <cgsi_plugin.h>
#include "ws-ifce/gsoap/fts3.nsmap"

using namespace boost;
using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace fts3::common;
using namespace std;

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int fts3::impltns__transferSubmit(soap *soap, tns3__TransferJob *_job, struct impltns__transferSubmitResponse &_param_3) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

	try {
		// since submitting requires (sometimes) delegation we need authorization on the delegation level
		AuthorizationManager::getInstance().authorize(
				soap,
				AuthorizationManager::DELEG,
				AuthorizationManager::dummy
			);

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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(
				soap,
				AuthorizationManager::DELEG,
				AuthorizationManager::dummy
			);

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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(
				soap,
				AuthorizationManager::DELEG,
				AuthorizationManager::dummy
			);

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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests' request" << commit;

	try {
		AuthorizationManager::Level lvl = AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER);
		RequestLister lister(soap, _inGivenStates);
		_param_7._listRequestsReturn = lister.list(lvl);

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int fts3::impltns__listRequests2(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string _forDN, string _forVO, struct impltns__listRequests2Response &_param_8) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests2' request" << commit;

	try {
		// todo the jobs should be listed accordingly to the authorization level
		AuthorizationManager::Level lvl = AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER);
		RequestLister lister(soap, _inGivenStates);
		_param_8._listRequests2Return = lister.list(lvl);

	} catch(Err& ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(soap, ex.what(), "TransferException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}


/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int fts3::impltns__getFileStatus(soap *soap, string _requestID, int _offset, int _limit, struct impltns__getFileStatusResponse &_param_9) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getFileStatus' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, _requestID);
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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getFileStatus2' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, _requestID);
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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getTransferJobStatus' request" << commit;

	try {
		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, _requestID);

		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
//		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(!fileStatuses.empty()){
			GSoapJobStatus status (soap, **fileStatuses.begin());
			_param_11._getTransferJobStatusReturn = status;
//			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

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
		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, _requestID);
		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
//		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if (!fileStatuses.empty()) {

			_param_12._getTransferJobSummaryReturn = soap_new_tns3__TransferJobSummary(soap, -1);
			GSoapJobStatus status (soap, **fileStatuses.begin());
			_param_12._getTransferJobSummaryReturn->jobStatus = status;

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

//			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

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
		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, _requestID);
		vector<JobStatus*> fileStatuses;
		DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID, fileStatuses);
//		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(!fileStatuses.empty()) {

			_param_13._getTransferJobSummary2Return = soap_new_tns3__TransferJobSummary2(soap, -1);
			GSoapJobStatus status (soap, **fileStatuses.begin());
			_param_13._getTransferJobSummary2Return->jobStatus = status;

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

//			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;

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
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getVersion' request" << commit;
	_param_21.getVersionReturn = VersionResolver::getInstance().getVersion();
	_param_21.getVersionReturn = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int fts3::impltns__getSchemaVersion(soap *soap, struct impltns__getSchemaVersionResponse &_param_22) {
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getSchemaVersion' request" << commit;
	_param_22.getSchemaVersionReturn = VersionResolver::getInstance().getSchema();
	_param_22.getSchemaVersionReturn = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int fts3::impltns__getInterfaceVersion(soap *soap, struct impltns__getInterfaceVersionResponse &_param_23) {
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getInterfaceVersion' request" << commit;
	_param_23.getInterfaceVersionReturn = VersionResolver::getInstance().getInterface();
	_param_23.getInterfaceVersionReturn = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int fts3::impltns__getServiceMetadata(soap *soap, string _key, struct impltns__getServiceMetadataResponse &_param_24) {
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getServiceMetadata' request" << commit;
	_param_24._getServiceMetadataReturn = VersionResolver::getInstance().getMetadata();
	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	return SOAP_OK;
}


/// Web service operation 'cancel' (returns error code or SOAP_OK)
int fts3::impltns__cancel(soap *soap, impltns__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct impltns__cancelResponse &_param_14) {

	try {
		CGsiAdapter cgsi (soap);
		string dn = cgsi.getClientDn();



		if (_requestIDs) {

			vector<string> &jobs = _requestIDs->item;
			std::vector<std::string>::iterator it;
			if (!jobs.empty()) {

				std::string jobId;
				for (it = jobs.begin(); it != jobs.end();) {
					// authorize the operation for each job ID
					AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::TRANSFER, *it);

					// check if the job ID exists
					scoped_ptr<TransferJobs> ptr (
							DBSingleton::instance().getDBObjectInstance()->getTransferJob(*it)
						);
					// if not throw an exception
					if (!ptr.get()) throw Err_Custom("Transfer job: " + *it + " does not exist!");

					vector<JobStatus*> status;
					DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(*it, status);

					if (!status.empty()) {
						// get transfer-job status
						string stat = (*status.begin())->jobStatus;
						// release the memory
						vector<JobStatus*>::iterator it_s;
						for (it_s = status.begin(); it_s != status.end(); it_s++) {
							delete *it_s;
						}
						// check if transfer-job is finished
						if (JobStatusHandler::getInstance().isTransferFinished(stat))
							throw Err_Custom("Transfer job: " + *it + " cannot be canceled (it is in " + stat + " state)");
					}

					jobId += *it;
					++it;
					if (it != jobs.end()) jobId += ", ";
				}

				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << "is canceling a transfer jobs: " << jobId << commit;

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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listVOManagers' request" << commit;

	_param_18._listVOManagersReturn = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(soap, -1);
	_param_18._listVOManagersReturn->item.push_back("default username");

	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int fts3::impltns__getRoles(soap *soap, struct impltns__getRolesResponse &_param_19) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRoles' request" << commit;

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

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRolesOf' request" << commit;

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
int fts3::impltns__debugSet(struct soap* soap, string _source, string _destination, bool _debug, struct impltns__debugSetResponse &_param_16) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'debugSet' request" << commit;

	try {

		CGsiAdapter cgsi(soap);
		string dn = cgsi.getClientDn();

		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn;
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << " is turning " << (_debug ? "on" : "off") << "the debug mode for " << _source;
		if (!_destination.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << " and " << _destination << " pair" << commit;
		}

		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::CONFIG, AuthorizationManager::dummy);
		DBSingleton::instance().getDBObjectInstance()->setDebugMode (
				_source,
				_destination,
				_debug ? "on" : "off"
			);

		string cmd = "fts3-debug-set ";
		cmd += (_debug ? "on " : "off ") + _source + " " + _destination;
		DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "debug");

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

int fts3::impltns__blacklist(soap* soap, string _type, string _subject, bool _blk, impltns__blacklistResponse &_param_18) {

	try {

		CGsiAdapter cgsi(soap);
		string dn = cgsi.getClientDn();

		AuthorizationManager::getInstance().authorize(soap, AuthorizationManager::CONFIG, AuthorizationManager::dummy);

		string cmd = "fts-set-blacklist " + _type + " " + _subject + (_blk ? " on" : " off");

		DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "blacklist");

		if (_type == "se") {

			if (_blk) {

				DBSingleton::instance().getDBObjectInstance()->blacklistSe(_subject, string(), dn);

			} else {

				DBSingleton::instance().getDBObjectInstance()->unblacklistSe(_subject);
			}

		} else if (_type == "dn") {

			if (_blk) {

				DBSingleton::instance().getDBObjectInstance()->blacklistDn(_subject, string(), dn);

			} else {

				DBSingleton::instance().getDBObjectInstance()->unblacklistDn(_subject);
			}

		} else {
			throw Err_Custom("Wrong type (it should be either 'se' or 'dn')!");
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

int fts3::impltns__prioritySet(soap* ctx, string job_id, int priority, impltns__prioritySetResponse &resp) {

	try {

		CGsiAdapter cgsi(ctx);
		string dn = cgsi.getClientDn();

		AuthorizationManager::getInstance().authorize(ctx, AuthorizationManager::TRANSFER, job_id);

		string cmd = "fts-set-priority " + job_id + " " + lexical_cast<string>(priority);

		DBSingleton::instance().getDBObjectInstance()->setPriority(job_id, priority);
		DBSingleton::instance().getDBObjectInstance()->auditConfiguration(dn, cmd, "set_priority");

	} catch(Err& ex) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << ex.what() << commit;
		soap_receiver_fault(ctx, ex.what(), "TransferException");
		return SOAP_FAULT;
	} catch (...) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown, the priority cannot be set"  << commit;
	    return SOAP_FAULT;
	}

	return SOAP_OK;
}


int fts3::log__GetLog(struct soap* soap, string jobId, struct log__GetLogResponse &_param_36) {

	// TODO read the log sources from DB
	vector<string> log_sources;
	log_sources.push_back("https://fts3-srv-node:8081");

	// tar the files together
	stringstream data;
	TarArchiver tar (data); // if we will use streaming TarArchiver has to be passed to the LogFileStreamer + use InternalLogRetriever

	vector<string>::iterator it;
	for (it = log_sources.begin(); it < log_sources.end(); it++) {

		::soap ctx;
		soap_init(&ctx);
		soap_cgsi_init(&ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
		soap_set_namespaces(&ctx, fts3_namespaces);

		log__GetLogInternalResponse resp;
		soap_call_log__GetLogInternal(&ctx, it->c_str(), 0, jobId, resp);

		// add each of the log files to the tar archive
		vector<string>::iterator n_it = resp.logs->lognames.begin();
		vector<log__Log *>::iterator d_it = resp.logs->logsdata.begin();

		for (; d_it < resp.logs->logsdata.end(); n_it++, d_it++) {
			stringstream ss;
			ss.write((char*) (*d_it)->xop__Include.__ptr, (*d_it)->xop__Include.__size);
			tar.putFile(ss, *n_it);
		}

		soap_destroy(&ctx);
		soap_end(&ctx);
		soap_done(&ctx);
	}

	tar.finish();

	// zip the tar
	stringstream zipstream;
	ZipCompressor zip(data, zipstream);
	zip.compress();
	zipstream.flush();

	// create a file stream
	log__Log* log = soap_new_log__Log(soap, -1);
	log->xop__Include.id = 0; // CID automatically generated by gSOAP engine
	log->xop__Include.type = "*/*"; // MIME type

	// determine the file size
    zipstream.seekg (0, ios::end);
    int size = zipstream.tellg();
    zipstream.seekg (0, ios::beg);
	log->xop__Include.__size = size;

	// set the data
	log->xop__Include.__ptr = (unsigned char *) soap_malloc(soap, size);
	while(!zipstream.eof()) {
		zipstream.read((char*) log->xop__Include.__ptr, 1024); // read one byte after another
	}

	_param_36.log = log;

	return SOAP_OK;
}

int fts3::log__GetLogInternal(struct soap* soap, string jobId, log__GetLogInternalResponse& resp) {

	// TODO read the paths from DB using jobId
	vector<string> logs;
	logs.push_back("/home/simonm/test1.txt");
	logs.push_back("/home/simonm/test2.txt");
	logs.push_back("/home/simonm/test3.txt");

	resp.logs = soap_new_log__LogInternal(soap, -1);

	vector<string>::iterator it;
	for (it = logs.begin(); it < logs.end(); it++) {

		// create a file stream
//		fstream* file = new fstream(it->c_str(), ios_base::in);
		// the stream will be closed and the memory will be freed by the readclose callback
//		log->xop__Include.__ptr = (unsigned char*) file;

		fstream file (it->c_str(), ios_base::in);
		if (!file) {
			soap_receiver_fault(soap, "The log file does not exist!", "LogFileStreamer Exception");
			return SOAP_FAULT;
		}

		log__Log* log = soap_new_log__Log(soap, -1);
		log->xop__Include.id = 0; // CID automatically generated by gSOAP engine
		log->xop__Include.type = "*/*"; // MIME type

		// determine the file size
	    file.seekg (0, ios::end);
	    int size = file.tellg();
	    file.seekg (0, ios::beg);
		log->xop__Include.__size = size;

		// set the data
		log->xop__Include.__ptr = (unsigned char *) soap_malloc(soap, size);
		while(!file.eof()) {
			file.read((char*) log->xop__Include.__ptr, 1024); // read one byte after another
		}

		resp.logs->logsdata.push_back(log);

		// determine the file name
		string::size_type pos = it->find_last_of("/");
		string logname;
		if (pos != string::npos) {
			logname = it->substr(pos + 1);
		} else {
			logname = *it;
		}
		resp.logs->lognames.push_back(logname);
	}

	return SOAP_OK;
}


