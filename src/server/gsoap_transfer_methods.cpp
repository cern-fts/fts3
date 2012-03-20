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
 */

#include "ws/gsoap_transfer_stubs.h"
#include "uuid_generator.h"

#include "db/generic/SingleDbInstance.h"
#include "config/serverconfig.h"

#include "ws/GSoapExceptionHandler.h"
#include "ws/JobSubmitter.h"
#include "ws/RequestLister.h"
#include "ws/JobStatusCopier.h"

#include "common/JobStatusHandler.h"
#include "common/logger.h"


using namespace boost;
using namespace db;
using namespace fts3::config;
using namespace fts3::ws;
using namespace fts3::common;
using namespace std;

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit(tns3__TransferJob *_job, struct impl__transferSubmitResponse &_param_3) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

	try {
		JobSubmitter submitter (this, _job, false);
		_param_3._transferSubmitReturn = submitter.submit();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (tns3__TransferException* ex) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit2(tns3__TransferJob *_job, struct impl__transferSubmit2Response &_param_4) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

	try {
		JobSubmitter submitter (this, _job, true);
		_param_4._transferSubmit2Return = submitter.submit();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_FAULT;

	} catch (tns3__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(tns3__TransferJob2 *_job, struct impl__transferSubmit3Response &_param_5) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

	try {
		JobSubmitter submitter (this, _job);
		_param_5._transferSubmit3Return = submitter.submit();

	} catch (string const &e) {

	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (tns3__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests(impl__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct impl__listRequestsResponse &_param_7) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests' request" << commit;

	try {
		RequestLister lister(this, _inGivenStates);
		_param_7._listRequestsReturn = lister.list();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
		return SOAP_SVR_FAULT;

	} catch (tns3__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests2(impl__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string _forDN, string _forVO, struct impl__listRequests2Response &_param_8) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests2' request" << commit;

	try {
		RequestLister lister(this, _inGivenStates);
		_param_8._listRequests2Return = lister.list();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
		return SOAP_SVR_FAULT;

	} catch (tns3__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}


/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus(string _requestID, int _offset, int _limit, struct impl__getFileStatusResponse &_param_9) {

	// dummy
	_param_9._getFileStatusReturn = soap_new_impl__ArrayOf_USCOREtns3_USCOREFileTransferStatus(this, -1);

	tns3__FileTransferStatus* status = soap_new_tns3__FileTransferStatus(this, -1);
	status->destSURL = soap_new_std__string(this, -1);
	status->logicalName = soap_new_std__string(this, -1);
	status->reason = soap_new_std__string(this, -1);
	status->reason_USCOREclass = soap_new_std__string(this, -1);
	status->sourceSURL = soap_new_std__string(this, -1);
	status->transferFileState = soap_new_std__string(this, -1);
	status->duration = 0;
	status->numFailures = 0;

	_param_9._getFileStatusReturn->item.push_back(status);

	return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus2(string _requestID, int _offset, int _limit, struct impl__getFileStatus2Response &_param_10) {

	// dummy
	_param_10._getFileStatus2Return = soap_new_impl__ArrayOf_USCOREtns3_USCOREFileTransferStatus2(this, -1);

	tns3__FileTransferStatus2* status = soap_new_tns3__FileTransferStatus2(this, -1);
	status->destSURL = soap_new_std__string(this, -1);
	status->logicalName = soap_new_std__string(this, -1);
	status->reason = soap_new_std__string(this, -1);
	status->reason_USCOREclass = soap_new_std__string(this, -1);
	status->sourceSURL = soap_new_std__string(this, -1);
	status->transferFileState = soap_new_std__string(this, -1);
	status->error_USCOREphase = soap_new_std__string(this, -1);
	status->error_USCOREscope = soap_new_std__string(this, -1);
	status->duration = 0;
	status->numFailures = 0;

	_param_10._getFileStatus2Return->item.push_back(status);

	return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobStatus(string _requestID, struct impl__getTransferJobStatusResponse &_param_11) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getTransferJobStatus' request" << commit;

	try{
		JobStatus* status =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID);
		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(status){
			_param_11._getTransferJobStatusReturn = JobStatusCopier::copyJobStatus(this, status);
			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;
			delete status;
		} else {
			tns3__NotExistsException* ex = GSoapExceptionHandler::createNotExistsException(this,
					"requestID <" + _requestID + "> was not found");
			throw ex;
		}

	} catch (string const &e) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (tns3__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary(string _requestID, struct impl__getTransferJobSummaryResponse &_param_12) {

	// dummy
	_param_12._getTransferJobSummaryReturn = soap_new_tns3__TransferJobSummary(this, -1);
	_param_12._getTransferJobSummaryReturn->numActive = 0;
	_param_12._getTransferJobSummaryReturn->numCanceled = 0;
	_param_12._getTransferJobSummaryReturn->numCanceling = 0;
	_param_12._getTransferJobSummaryReturn->numCatalogFailed = 0;
	_param_12._getTransferJobSummaryReturn->numDone = 0;
	_param_12._getTransferJobSummaryReturn->numFailed = 0;
	_param_12._getTransferJobSummaryReturn->numFinished = 0;
	_param_12._getTransferJobSummaryReturn->numHold = 0;
	_param_12._getTransferJobSummaryReturn->numPending = 0;
	_param_12._getTransferJobSummaryReturn->numRestarted = 0;
	_param_12._getTransferJobSummaryReturn->numSubmitted = 0;
	_param_12._getTransferJobSummaryReturn->numWaiting = 0;

	_param_12._getTransferJobSummaryReturn->jobStatus = soap_new_tns3__JobStatus(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->clientDN = soap_new_std__string(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->jobID = soap_new_std__string(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->jobStatus = soap_new_std__string(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->reason = soap_new_std__string(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->voName = soap_new_std__string(this, -1);
	_param_12._getTransferJobSummaryReturn->jobStatus->submitTime = 0;
	_param_12._getTransferJobSummaryReturn->jobStatus->numFiles = 0;
	_param_12._getTransferJobSummaryReturn->jobStatus->priority = 0;

	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary2(string _requestID, struct impl__getTransferJobSummary2Response &_param_13) {

	// dummy
	_param_13._getTransferJobSummary2Return = soap_new_tns3__TransferJobSummary2(this, -1);
	_param_13._getTransferJobSummary2Return->numActive = 0;
	_param_13._getTransferJobSummary2Return->numAwaitingPrestage = 0;
	_param_13._getTransferJobSummary2Return->numCanceled = 0;
	_param_13._getTransferJobSummary2Return->numCanceling = 0;
	_param_13._getTransferJobSummary2Return->numFinishing = 0;
	_param_13._getTransferJobSummary2Return->numCatalogFailed = 0;
	_param_13._getTransferJobSummary2Return->numDone = 0;
	_param_13._getTransferJobSummary2Return->numFailed = 0;
	_param_13._getTransferJobSummary2Return->numFinished = 0;
	_param_13._getTransferJobSummary2Return->numHold = 0;
	_param_13._getTransferJobSummary2Return->numPrestaging = 0;
	_param_13._getTransferJobSummary2Return->numPending = 0;
	_param_13._getTransferJobSummary2Return->numRestarted = 0;
	_param_13._getTransferJobSummary2Return->numSubmitted = 0;
	_param_13._getTransferJobSummary2Return->numReady = 0;
	_param_13._getTransferJobSummary2Return->numWaiting = 0;
	_param_13._getTransferJobSummary2Return->numWaitingCatalogRegistration = 0;
	_param_13._getTransferJobSummary2Return->numWaitingCatalogResolution = 0;
	_param_13._getTransferJobSummary2Return->numWaitingPrestage = 0;

	_param_13._getTransferJobSummary2Return->jobStatus = soap_new_tns3__JobStatus(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->clientDN = soap_new_std__string(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->jobID = soap_new_std__string(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->jobStatus = soap_new_std__string(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->reason = soap_new_std__string(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->voName = soap_new_std__string(this, -1);
	_param_13._getTransferJobSummary2Return->jobStatus->submitTime = 0;
	_param_13._getTransferJobSummary2Return->jobStatus->numFiles = 0;
	_param_13._getTransferJobSummary2Return->jobStatus->priority = 0;

	return SOAP_OK;
}



/// Web service operation 'getVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getVersion(struct impl__getVersionResponse &_param_21) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getVersion' request" << commit;
	_param_21.getVersionReturn = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getSchemaVersion(struct impl__getSchemaVersionResponse &_param_22) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getSchemaVersion' request" << commit;
	_param_22.getSchemaVersionReturn = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getInterfaceVersion(struct impl__getInterfaceVersionResponse &_param_23) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getInterfaceVersion' request" << commit;
	_param_23.getInterfaceVersionReturn = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getServiceMetadata(string _key, struct impl__getServiceMetadataResponse &_param_24) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getServiceMetadata' request" << commit;
	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	return SOAP_OK;
}


/// Web service operation 'cancel' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::cancel(impl__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct impl__cancelResponse &_param_14) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'cancel' request" << commit;

	if (_requestIDs) {
		vector<string> &jobs = _requestIDs->item;
		if (!jobs.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Jobs that have been canceled:" << commit;
			vector<string>::iterator it;
			for (it = jobs.begin(); it < jobs.end(); it++) {
				FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << *it << commit;
			}
		}
	}

	return SOAP_OK;
}

/// Web service operation 'setJobPriority' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::setJobPriority(string _requestID, int _priority, struct impl__setJobPriorityResponse &_param_15) {
	return SOAP_OK;
}

/// Web service operation 'addVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::addVOManager(string _VOName, string _principal, struct impl__addVOManagerResponse &_param_16) {
	return SOAP_OK;
}

/// Web service operation 'removeVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::removeVOManager(string _VOName, string _principal, struct impl__removeVOManagerResponse &_param_17) {
	return SOAP_OK;
}

/// Web service operation 'listVOManagers' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listVOManagers(string _VOName, struct impl__listVOManagersResponse &_param_18) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listVOManagers' request" << commit;

	_param_18._listVOManagersReturn = soap_new_impl__ArrayOf_USCOREsoapenc_USCOREstring(this, -1);
	_param_18._listVOManagersReturn->item.push_back("default username");

	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRoles(struct impl__getRolesResponse &_param_19) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRoles' request" << commit;

	_param_19.getRolesReturn = soap_new_tns3__Roles(this, -1);

	_param_19.getRolesReturn->clientDN = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->clientDN = "clientDN";
	_param_19.getRolesReturn->serviceAdmin = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->serviceAdmin = "serviceAdmin";
	_param_19.getRolesReturn->submitter = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->submitter = "submitter";

	tns3__StringPair* pair = soap_new_tns3__StringPair(this, -1);
	pair->string1 = soap_new_std__string(this, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(this, -1);
	*pair->string2 = "string2";
	_param_19.getRolesReturn->VOManager.push_back(pair);

	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRolesOf(string _otherDN, struct impl__getRolesOfResponse &_param_20) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRolesOf' request" << commit;

	_param_20._getRolesOfReturn = soap_new_tns3__Roles(this, -1);

	_param_20._getRolesOfReturn->clientDN = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->clientDN = "clientDN";
	_param_20._getRolesOfReturn->serviceAdmin = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->serviceAdmin = "serviceAdmin";
	_param_20._getRolesOfReturn->submitter = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->submitter = "submitter";

	tns3__StringPair* pair = soap_new_tns3__StringPair(this, -1);
	pair->string1 = soap_new_std__string(this, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(this, -1);
	*pair->string2 = "string2";
	_param_20._getRolesOfReturn->VOManager.push_back(pair);

	return SOAP_OK;
}
