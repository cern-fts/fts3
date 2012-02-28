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
#include "ws/FtsServiceTask.h"

#include "common/logger.h"

using namespace fts::ws;
using namespace boost;
using namespace db;
using namespace fts3::config;

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit(transfer__TransferJob *_job, struct fts__transferSubmitResponse &_param_3) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit' request" << commit;

	if (_job == 0) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "The job was not defined" << commit;
		transfer__InvalidArgumentException ex;
		throw ex;
	}

	string id = UuidGenerator::generateUUID();
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Generated uuid " << id << commit;

	try{
	    const string requestID = id;
	    const string dn = "/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
	    const string vo = string("dteam");

	    FtsServiceTask srvtask;

	    vector<src_dest_checksum_tupple> jobs = srvtask.getJobs(_job);
	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;

	    string sourceSpaceTokenDescription = "";

	    string cred = "";
	    if (_job->credential) {
	    	cred = *_job->credential;
	    }
	    int copyPinLifeTime = 1;

	    vector<string> params = srvtask.getParams(_job->jobParams, copyPinLifeTime);

	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter's vector has been created" << commit;

	    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, jobs, params[0],
	                                 dn, cred, vo, params[1],
	                                 params[2], params[3], params[4],
	                                 params[5], sourceSpaceTokenDescription, params[6], copyPinLifeTime,
	                                 params[7], params[8]);

	    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The job has been submitted" << commit;
	  }
	catch (string const &e)
	  {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;
	  }
	_param_3._transferSubmitReturn = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit2(transfer__TransferJob *_job, struct fts__transferSubmit2Response &_param_4) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit2' request" << commit;

	if (_job == 0) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "The job was not defined" << commit;
		transfer__InvalidArgumentException ex;
		throw ex;
	}

	string id = UuidGenerator::generateUUID();
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Generated uuid " << id << commit;

	try{
	    const string requestID = id;
	    const string dn = "/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
	    const string vo = string("dteam");

	    FtsServiceTask srvtask;

	    vector<src_dest_checksum_tupple> jobs = srvtask.getJobs(_job);
	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;

	    string sourceSpaceTokenDescription = "";

	    string cred = "";
	    if (_job->credential) {
	    	cred = *_job->credential;
	    }

	    int copyPinLifeTime = 1;

	    vector<string> params = srvtask.getParams(_job->jobParams, copyPinLifeTime);
	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter's vector has been created" << commit;

	    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, jobs, params[0],
	                                 dn, cred, vo, params[1],
	                                 params[2], params[3], params[4],
	                                 params[5], sourceSpaceTokenDescription, params[6], copyPinLifeTime,
	                                 params[7], params[8]);

	    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The job has been submitted" << commit;
	  }
	catch (string const &e)
	  {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;
	  }

	_param_4._transferSubmit2Return = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(transfer__TransferJob2 *_job, struct fts__transferSubmit3Response &_param_5) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'transferSubmit3' request" << commit;

	if (_job == 0) {

		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "The job was not defined" << commit;
		transfer__InvalidArgumentException ex;
		throw ex;
	}

	string id = UuidGenerator::generateUUID();
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Generated uuid " << id << commit;

	try{
	    const string requestID = id;
	    const string dn = "/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
	    const string vo = string("dteam");

	    FtsServiceTask srvtask;

	    vector<src_dest_checksum_tupple> jobs = srvtask.getJobs2(_job);
	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;

	    string sourceSpaceTokenDescription = "";

	    string cred = "";
	    if (_job->credential) {
	    	cred = *_job->credential;
	    }
	    int copyPinLifeTime = 1;

	    vector<string> params = srvtask.getParams(_job->jobParams, copyPinLifeTime);
	    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter's vector has been created" << commit;

	    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, jobs, params[0],
	                                 dn, cred, vo, params[1],
	                                 params[2], params[3], params[4],
	                                 params[5], sourceSpaceTokenDescription, params[6], copyPinLifeTime,
	                                 params[7], params[8]);

	    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The job has been submitted" << commit;
	  }
	catch (string const &e)
	  {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;
	  }
	_param_5._transferSubmit3Return = id;

	return SOAP_OK;
}

/// Web service operation 'submit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::submit(transfer__TransferJob *_job, struct fts__submitResponse &_param_6) {
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
		JobStatus* record =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID);
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The job status has been read" << commit;

		if(record){

			_param_11._getTransferJobStatusReturn = soap_new_transfer__JobStatus(this, -1);

			_param_11._getTransferJobStatusReturn->jobID = soap_new_std__string(this, -1);
			*_param_11._getTransferJobStatusReturn->jobID = record->jobID;

			_param_11._getTransferJobStatusReturn->jobStatus = soap_new_std__string(this, -1);
			*_param_11._getTransferJobStatusReturn->jobStatus = record->jobStatus;

			_param_11._getTransferJobStatusReturn->clientDN = soap_new_std__string(this, -1);
			*_param_11._getTransferJobStatusReturn->clientDN = record->clientDN;

			_param_11._getTransferJobStatusReturn->reason = soap_new_std__string(this, -1);
			*_param_11._getTransferJobStatusReturn->reason = record->reason;

			_param_11._getTransferJobStatusReturn->voName = soap_new_std__string(this, -1);
			*_param_11._getTransferJobStatusReturn->voName = record->voName;

			_param_11._getTransferJobStatusReturn->submitTime = record->submitTime;
			_param_11._getTransferJobStatusReturn->numFiles = record->numFiles;
			_param_11._getTransferJobStatusReturn->priority = record->priority;

			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The response hav been created" << commit;

			delete record;
		}
	} catch (string const &e) {
	    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "An exception has been thrown: " << e << commit;
	    return SOAP_SVR_FAULT;
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



/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct fts__listRequestsResponse &_param_7) {
	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests2(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, string _forDN, string _forVO, struct fts__listRequests2Response &_param_8) {
	return SOAP_OK;
}



/// Web service operation 'cancel' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::cancel(fts__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct fts__cancelResponse &_param_14) {
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
	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRoles(struct fts__getRolesResponse &_param_19) {
	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRolesOf(string _otherDN, struct fts__getRolesOfResponse &_param_20) {
	return SOAP_OK;
}

/// Web service operation 'placementSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::placementSubmit(transfer__PlacementJob *_job, struct fts__placementSubmitResponse &_param_1) {
	return SOAP_OK;
}

/// Web service operation 'placementSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::placementSubmit2(transfer__PlacementJob *_job, struct fts__placementSubmit2Response &_param_2) {
	return SOAP_OK;
}
