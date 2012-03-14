/*
 * WebServerMock.cpp
 *
 *  Created on: Feb 24, 2012
 *      Author: simonm
 */

#include "WebServerMock.h"
#include "uuid_generator.h"

using namespace fts3::ws;


WebServerMock* WebServerMock::me = 0;


WebServerMock::WebServerMock() {
	tid = 0;
}

WebServerMock::~WebServerMock() {
	if (me) {
		delete me;
	}

	if (tid) {
		pthread_cancel(tid);
	}
}

WebServerMock* WebServerMock::getInstance() {

	// lazy loading
	if (!me) {
		me = new WebServerMock();
	}
	return me;
}

void WebServerMock::run(int port) {
	this->port = port;
	pthread_create(&tid, NULL, processRequest, 0);

	int old;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old);

}

void* WebServerMock::processRequest(void* ptr) {

	WebServerMock* me = getInstance();
	FileTransferSoapBindingService service;
	service.run(me->port);

	return 0;
}





/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit(transfer__TransferJob *_job, struct fts__transferSubmitResponse &_param_3) {

	WebServerMock* me = WebServerMock::getInstance();

	string id = UuidGenerator::generateUUID();
	_param_3._transferSubmitReturn = id;
	me->id = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit2(transfer__TransferJob *_job, struct fts__transferSubmit2Response &_param_4) {

	if (_job == 0) {
		transfer__InvalidArgumentException ex;
		throw ex;
	}

	WebServerMock* me = WebServerMock::getInstance();

	string id = UuidGenerator::generateUUID();
	_param_4._transferSubmit2Return = id;
	me->id = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(transfer__TransferJob2 *_job, struct fts__transferSubmit3Response &_param_5) {

	WebServerMock* me = WebServerMock::getInstance();

	string id = UuidGenerator::generateUUID();
	_param_5._transferSubmit3Return = id;
	me->id = id;
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

	WebServerMock* me = WebServerMock::getInstance();

	_param_21.getVersionReturn = "3.7.6-1";
	me->version = "3.7.6-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getSchemaVersion(struct fts__getSchemaVersionResponse &_param_22) {

	WebServerMock* me = WebServerMock::getInstance();

	_param_22.getSchemaVersionReturn = "3.5.0";
	me->schema = "3.5.0";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getInterfaceVersion(struct fts__getInterfaceVersionResponse &_param_23) {

	WebServerMock* me = WebServerMock::getInstance();

	_param_23.getInterfaceVersionReturn = "3.7.0";
	me->interface = "3.7.0";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getServiceMetadata(std::string _key, struct fts__getServiceMetadataResponse &_param_24) {

	WebServerMock* me = WebServerMock::getInstance();

	_param_24._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";
	me->metadata = "glite-data-fts-service-3.7.6-1";
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

