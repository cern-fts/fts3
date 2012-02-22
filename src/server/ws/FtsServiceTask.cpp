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


/*
 * FtsService.cpp
 *
 *  Created on: Feb 18, 2012
 *      Author: simonm
 */

#include "FtsServiceTask.h"
#include "GsoapStubs.h"
#include "UuidGenerator.h"

using namespace fts::ws;

FtsServiceTask::FtsServiceTask() {
}

FtsServiceTask::~FtsServiceTask() {
}

void FtsServiceTask::operator ()(FileTransferSoapBindingService* copy) {

	// serve the request
	copy->serve();

	// free the resources and memory
	soap_destroy(copy);
	soap_end(copy);
	soap_done(copy);
	delete copy;
}

FtsServiceTask& FtsServiceTask::operator= (const FtsServiceTask& other) {
	return *this;
}

FileTransferSoapBindingService* FtsServiceTask::copyService(FileTransferSoapBindingService& srv) {

	// allocate memory
	FileTransferSoapBindingService* copy = new FileTransferSoapBindingService();
	// copy the object
	*copy = srv;
	// indicate that the object is a copy
    copy->state = SOAP_COPY;
    // set default values
    copy->error = SOAP_OK;
    copy->userid = NULL;
    copy->passwd = NULL;
    copy->nlist = NULL;
    copy->blist = NULL;
    copy->clist = NULL;
    copy->alist = NULL;
    copy->attributes = NULL;
    copy->labbuf = NULL;
    copy->lablen = 0;
    copy->labidx = 0;
    copy->local_namespaces = NULL;
    copy->plugins = NULL;
    copy->cookies = NULL;
    copy->header = NULL;
    copy->fault = NULL;
    copy->action = NULL;
    // set the namespaces
    soap_set_namespaces(copy, srv.local_namespaces);
    // initialize the copy
    copy->FileTransferSoapBindingService_init(srv.imode, srv.omode);

	return copy;
}



/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit(transfer__TransferJob *_job, struct fts__transferSubmitResponse &_param_3) {
	string id = UuidGenerator::generateUUID();
	cout << "Job ID: " << id << endl;
	_param_3._transferSubmitReturn = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit2(transfer__TransferJob *_job, struct fts__transferSubmit2Response &_param_4) {

	if (_job == 0) {
		transfer__InvalidArgumentException ex;
		throw ex;
	}

	string id = UuidGenerator::generateUUID();
	cout << "Job ID: " << id << endl;
	_param_4._transferSubmit2Return = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(transfer__TransferJob2 *_job, struct fts__transferSubmit3Response &_param_5) {
	string id = UuidGenerator::generateUUID();
	cout << "Job ID: " << id << endl;
	_param_5._transferSubmit3Return = id;
	return SOAP_OK;
}

/// Web service operation 'submit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::submit(transfer__TransferJob *_job, struct fts__submitResponse &_param_6) {
	return SOAP_OK;
}



/// Web service operation 'getFileStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus(std::string _requestID, int _offset, int _limit, struct fts__getFileStatusResponse &_param_9) {
	return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus2(std::string _requestID, int _offset, int _limit, struct fts__getFileStatus2Response &_param_10) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobStatus(std::string _requestID, struct fts__getTransferJobStatusResponse &_param_11) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary(std::string _requestID, struct fts__getTransferJobSummaryResponse &_param_12) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobSummary2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobSummary2(std::string _requestID, struct fts__getTransferJobSummary2Response &_param_13) {
	return SOAP_OK;
}



/// Web service operation 'getVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getVersion(struct fts__getVersionResponse &_param_21) {
	_param_21.getVersionReturn = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getSchemaVersion(struct fts__getSchemaVersionResponse &_param_22) {
	_param_22.getSchemaVersionReturn = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getInterfaceVersion(struct fts__getInterfaceVersionResponse &_param_23) {
	_param_23.getInterfaceVersionReturn = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getServiceMetadata(std::string _key, struct fts__getServiceMetadataResponse &_param_24) {
	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	return SOAP_OK;
}



/// Web service operation 'listRequests' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, struct fts__listRequestsResponse &_param_7) {
	return SOAP_OK;
}

/// Web service operation 'listRequests2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listRequests2(fts__ArrayOf_USCOREsoapenc_USCOREstring *_inGivenStates, std::string _forDN, std::string _forVO, struct fts__listRequests2Response &_param_8) {
	return SOAP_OK;
}



/// Web service operation 'cancel' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::cancel(fts__ArrayOf_USCOREsoapenc_USCOREstring *_requestIDs, struct fts__cancelResponse &_param_14) {
	return SOAP_OK;
}

/// Web service operation 'setJobPriority' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::setJobPriority(std::string _requestID, int _priority, struct fts__setJobPriorityResponse &_param_15) {
	return SOAP_OK;
}

/// Web service operation 'addVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::addVOManager(std::string _VOName, std::string _principal, struct fts__addVOManagerResponse &_param_16) {
	return SOAP_OK;
}

/// Web service operation 'removeVOManager' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::removeVOManager(std::string _VOName, std::string _principal, struct fts__removeVOManagerResponse &_param_17) {
	return SOAP_OK;
}

/// Web service operation 'listVOManagers' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::listVOManagers(std::string _VOName, struct fts__listVOManagersResponse &_param_18) {
	return SOAP_OK;
}

/// Web service operation 'getRoles' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRoles(struct fts__getRolesResponse &_param_19) {
	return SOAP_OK;
}

/// Web service operation 'getRolesOf' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getRolesOf(std::string _otherDN, struct fts__getRolesOfResponse &_param_20) {
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
