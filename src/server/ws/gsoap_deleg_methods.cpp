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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */


#include "ws/gsoap_stubs.h"
#include "ws/GSoapDelegationHandler.h"

#include "common/logger.h"


using namespace std;
using namespace fts3::common;
using namespace fts3::ws;


int fts3::delegation__getProxyReq(struct soap* soap, std::string _delegationID, struct deleg__getProxyReqResponse &_param_4) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getProxyReq' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		_param_4._getProxyReqReturn = handler.getProxyReq(_delegationID);

	} catch (string& ex) {
		// TODO update GSoapExceptionHandler to handle _deleg__DelegationException
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__getNewProxyReq(struct soap* soap, struct deleg__getNewProxyReqResponse &_param_5) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getNewProxyReq' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		_param_5.getNewProxyReqReturn = handler.getNewProxyReq();

	} catch (string& ex) {
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__renewProxyReq(struct soap* soap, std::string _delegationID, struct deleg__renewProxyReqResponse &_param_6) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__renewProxyReq' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		_param_6._renewProxyReqReturn = handler.renewProxyReq(_delegationID);

	} catch(string& ex) {
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__putProxy(struct soap* soap, std::string _delegationID, std::string _proxy, struct deleg__putProxyResponse &_param_7) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__putProxy' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		handler.putProxy(_delegationID, _proxy);

	} catch (string& ex) {
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__getTerminationTime(struct soap* soap, std::string _delegationID, struct deleg__getTerminationTimeResponse &_param_8) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getTerminationTime' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		_param_8._getTerminationTimeReturn = handler.getTerminationTime(_delegationID);

	} catch (string& ex) {
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__destroy(struct soap* soap, std::string _delegationID, struct deleg__destroyResponse &_param_9) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__destroy' request" << commit;

	GSoapDelegationHandler handler(soap);
	try {
		handler.destroy(_delegationID);

	} catch(string& ex) {
		soap_receiver_fault(soap, ex.c_str(), "DelegationException");
		return SOAP_FAULT;
	}

	return SOAP_OK;
}

int fts3::delegation__getVersion(struct soap* soap, struct deleg__getVersionResponse &_param_1) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getVersion' request" << commit;
	_param_1.getVersionReturn = "3.7.6-1";

	return SOAP_OK;
}

int fts3::delegation__getInterfaceVersion(struct soap* soap, struct deleg__getInterfaceVersionResponse &_param_2) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getInterfaceVersion' request" << commit;
	_param_2.getInterfaceVersionReturn = "3.7.0";

	return SOAP_OK;
}

int fts3::delegation__getServiceMetadata(struct soap* soap, std::string _key, struct deleg__getServiceMetadataResponse &_param_3) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'deleg__getServiceMetadata' request" << commit;
	_param_3._getServiceMetadataReturn = "glite-data-fts-service-3.7.6-1";

	return SOAP_OK;
}

