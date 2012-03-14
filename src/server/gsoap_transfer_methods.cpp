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

#include "gsoap_stubs.h"
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
int FileTransferSoapBindingService::transferSubmit(transfer__TransferJob *_job, struct fts__transferSubmitResponse &_param_3) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

	try {
		JobSubmitter submitter (this, _job, false);
		_param_3._transferSubmitReturn = submitter.submit();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (transfer__TransferException* ex) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit2(transfer__TransferJob *_job, struct fts__transferSubmit2Response &_param_4) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

	try {
		JobSubmitter submitter (this, _job, true);
		_param_4._transferSubmit2Return = submitter.submit();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_FAULT;

	} catch (transfer__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(transfer__TransferJob2 *_job, struct fts__transferSubmit3Response &_param_5) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

	try {
		JobSubmitter submitter (this, _job);
		_param_5._transferSubmit3Return = submitter.submit();

	} catch (string const &e) {

	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (transfer__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct fts__listRequestsResponse &_param_7) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests' request" << commit;

	try {
		RequestLister lister(this, _inGivenStates);
		_param_7._listRequestsReturn = lister.list();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
		return SOAP_SVR_FAULT;

	} catch (transfer__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests2(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string _forDN, string _forVO, struct fts__listRequests2Response &_param_8) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listRequests2' request" << commit;

	try {
		RequestLister lister(this, _inGivenStates);
		_param_8._listRequests2Return = lister.list();

	} catch (string const &e) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
		return SOAP_SVR_FAULT;

	} catch (transfer__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}


/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus(string _requestID, int _offset, int _limit, struct fts__getFileStatusResponse &_param_9) {
	return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus2(string _requestID, int _offset, int _limit, struct fts__getFileStatus2Response &_param_10) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobStatus(string _requestID, struct fts__getTransferJobStatusResponse &_param_11) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getTransferJobStatus' request" << commit;

	try{
		JobStatus* status =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID);
		FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The job status has been read" << commit;

		if(status){
			_param_11._getTransferJobStatusReturn = JobStatusCopier::copyJobStatus(this, status);
			FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "The response has been created" << commit;
			delete status;
		} else {
			transfer__NotExistsException* ex = GSoapExceptionHandler::createNotExistsException(this,
					"requestID <" + _requestID + "> was not found");
			throw ex;
		}

	} catch (string const &e) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;

	} catch (transfer__TransferException* ex) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been caught: " << *ex->message << commit;
		GSoapExceptionHandler exHandler(this, ex);
		exHandler.handle();
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary(string _requestID, struct fts__getTransferJobSummaryResponse &_param_12) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary2(string _requestID, struct fts__getTransferJobSummary2Response &_param_13) {
	return SOAP_OK;
}



/// Web service operation 'getVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getVersion(struct fts__getVersionResponse &_param_21) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getVersion' request" << commit;
	_param_21.getVersionReturn = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getSchemaVersion(struct fts__getSchemaVersionResponse &_param_22) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getSchemaVersion' request" << commit;
	_param_22.getSchemaVersionReturn = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getInterfaceVersion(struct fts__getInterfaceVersionResponse &_param_23) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getInterfaceVersion' request" << commit;
	_param_23.getInterfaceVersionReturn = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getServiceMetadata(string _key, struct fts__getServiceMetadataResponse &_param_24) {
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getServiceMetadata' request" << commit;
	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	return SOAP_OK;
}


/// Web service operation 'cancel' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::cancel(fts__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct fts__cancelResponse &_param_14) {

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
int FileTransferSoapBindingService::setJobPriority(string _requestID, int _priority, struct fts__setJobPriorityResponse &_param_15) {
	return SOAP_OK;
}

/// Web service operation 'addVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::addVOManager(string _VOName, string _principal, struct fts__addVOManagerResponse &_param_16) {
	return SOAP_OK;
}

/// Web service operation 'removeVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::removeVOManager(string _VOName, string _principal, struct fts__removeVOManagerResponse &_param_17) {
	return SOAP_OK;
}

/// Web service operation 'listVOManagers' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listVOManagers(string _VOName, struct fts__listVOManagersResponse &_param_18) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'listVOManagers' request" << commit;

	_param_18._listVOManagersReturn = soap_new_fts__ArrayOf_USCOREsoapenc_USCOREstring(this, -1);
	_param_18._listVOManagersReturn->item.push_back("default username");

	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRoles(struct fts__getRolesResponse &_param_19) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRoles' request" << commit;

	_param_19.getRolesReturn = soap_new_transfer__Roles(this, -1);

	_param_19.getRolesReturn->clientDN = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->clientDN = "clientDN";
	_param_19.getRolesReturn->serviceAdmin = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->serviceAdmin = "serviceAdmin";
	_param_19.getRolesReturn->submitter = soap_new_std__string(this, -1);
	*_param_19.getRolesReturn->submitter = "submitter";

	transfer__StringPair* pair = soap_new_transfer__StringPair(this, -1);
	pair->string1 = soap_new_std__string(this, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(this, -1);
	*pair->string2 = "string2";
	_param_19.getRolesReturn->VOManager.push_back(pair);

	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRolesOf(string _otherDN, struct fts__getRolesOfResponse &_param_20) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getRolesOf' request" << commit;

	_param_20._getRolesOfReturn = soap_new_transfer__Roles(this, -1);

	_param_20._getRolesOfReturn->clientDN = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->clientDN = "clientDN";
	_param_20._getRolesOfReturn->serviceAdmin = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->serviceAdmin = "serviceAdmin";
	_param_20._getRolesOfReturn->submitter = soap_new_std__string(this, -1);
	*_param_20._getRolesOfReturn->submitter = "submitter";

	transfer__StringPair* pair = soap_new_transfer__StringPair(this, -1);
	pair->string1 = soap_new_std__string(this, -1);
	*pair->string1 = "string1";
	pair->string2 = soap_new_std__string(this, -1);
	*pair->string2 = "string2";
	_param_20._getRolesOfReturn->VOManager.push_back(pair);

	return SOAP_OK;
}
