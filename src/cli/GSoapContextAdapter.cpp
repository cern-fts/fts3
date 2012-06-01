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
 * GSoapContexAdapter.cpp
 *
 *  Created on: May 30, 2012
 *      Author: simonm
 */

#include "GSoapContextAdapter.h"
#include "ws-ifce/gsoap/fts3.nsmap"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#include <cgsi_plugin.h>

using namespace boost;
using namespace fts3::cli;

GSoapContextAdapter::GSoapContextAdapter(string endpoint): endpoint(endpoint), ctx(soap_new()) {

}

GSoapContextAdapter::~GSoapContextAdapter() {
	soap_destroy(ctx);
	soap_end(ctx);
	soap_free(ctx);
}

void GSoapContextAdapter::init() {

    int  err = 0;

    // initialize cgsi plugin
    if (endpoint.find("https") == 0) {
    	err = soap_cgsi_init(ctx,  CGSI_OPT_DISABLE_NAME_CHECK | CGSI_OPT_SSL_COMPATIBLE);
    } else if (endpoint.find("httpg") == 0) {
    	err = soap_cgsi_init(ctx, CGSI_OPT_DISABLE_NAME_CHECK);
    }

    if (err) {
    	handleSoapFault("Failed to initialize the GSI plugin.");
    }

    // set the namespaces
	if (soap_set_namespaces(ctx, fts3_namespaces)) {
		handleSoapFault("Failed to set SOAP namespaces.");
	}

	// request the information about the FTS3 service
	impltns__getInterfaceVersionResponse ivresp;
	err = soap_call_impltns__getInterfaceVersion(ctx, endpoint.c_str(), 0, ivresp);
	if (!err) {
		interface = ivresp.getInterfaceVersionReturn;
		setInterfaceVersion(interface);
	} else {
		handleSoapFault("Failed to determine the interface version of the service: getInterfaceVersion.");
	}

	impltns__getVersionResponse vresp;
	err = soap_call_impltns__getVersion(ctx, endpoint.c_str(), 0, vresp);
	if (!err) {
		version = vresp.getVersionReturn;
	} else {
		handleSoapFault("Failed to determine the version of the service: getVersion.");
	}

	impltns__getSchemaVersionResponse sresp;
	err = soap_call_impltns__getSchemaVersion(ctx, endpoint.c_str(), 0, sresp);
	if (!err) {
		schema = sresp.getSchemaVersionReturn;
	} else {
		handleSoapFault("Failed to determine the schema version of the service: getSchemaVersion.");
	}

	impltns__getServiceMetadataResponse mresp;
	err = soap_call_impltns__getServiceMetadata(ctx, endpoint.c_str(), 0, "feature.string", mresp);
	if (!err) {
		metadata = mresp._getServiceMetadataReturn;
	} else {
		handleSoapFault("Failed to determine the service metadata of the service: getServiceMetadataReturn.");
	}
}

void GSoapContextAdapter::printInfo() {
	// print the info
	cout << "# Using endpoint: " << endpoint << endl;
	cout << "# Service version: " << version << endl;
	cout << "# Interface version: " << interface << endl;
	cout << "# Schema version: " << schema << endl;
	cout << "# Service features: " << metadata << endl;
	cout << "# Client version: TODO" << endl;
	cout << "# Client interface version: TODO" << endl;
}

void GSoapContextAdapter::transferSubmit3 (tns3__TransferJob2* job, impltns__transferSubmit3Response& resp) {

	if (soap_call_impltns__transferSubmit3(ctx, endpoint.c_str(), 0, job, resp))
		handleSoapFault("Failed to submit transfer: transferSubmit3.");
}

void GSoapContextAdapter::transferSubmit2 (tns3__TransferJob* job, impltns__transferSubmit2Response& resp) {

	if (soap_call_impltns__transferSubmit2(ctx, endpoint.c_str(), 0, job, resp))
		handleSoapFault("Failed to submit transfer: transferSubmit2.");
}

void GSoapContextAdapter::transferSubmit (tns3__TransferJob* job, impltns__transferSubmitResponse& resp) {
	if (soap_call_impltns__transferSubmit(ctx, endpoint.c_str(), 0, job, resp))
		handleSoapFault("Failed to submit transfer: transferSubmit.");
}

void GSoapContextAdapter::getTransferJobStatus (string jobId, impltns__getTransferJobStatusResponse& resp) {
	if (soap_call_impltns__getTransferJobStatus(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobStatus.");
}

void GSoapContextAdapter::cancel(impltns__ArrayOf_USCOREsoapenc_USCOREstring* rqst, impltns__cancelResponse& resp) {
	if (soap_call_impltns__cancel(ctx, endpoint.c_str(), 0, rqst, resp))
		handleSoapFault("Failed to cancel jobs: cancel.");
}

void GSoapContextAdapter::getRoles (impltns__getRolesResponse& resp) {
	if (soap_call_impltns__getRoles(ctx, endpoint.c_str(), 0, resp))
		handleSoapFault("Failed to get roles: getRoles.");
}

void GSoapContextAdapter::getRolesOf (string dn, impltns__getRolesOfResponse& resp) {
	if (soap_call_impltns__getRolesOf(ctx, endpoint.c_str(), 0, dn, resp))
		handleSoapFault("Failed to get roles: getRolesOf.");
}

void GSoapContextAdapter::listRequests2 (impltns__ArrayOf_USCOREsoapenc_USCOREstring* array, string dn, string vo, impltns__listRequests2Response& resp) {
	if (soap_call_impltns__listRequests2(ctx, endpoint.c_str(), 0, array, dn, vo, resp))
		handleSoapFault("Failed to list requests: listRequests2.");
}

void GSoapContextAdapter::listRequests (impltns__ArrayOf_USCOREsoapenc_USCOREstring* array, impltns__listRequestsResponse& resp) {
	if (soap_call_impltns__listRequests(ctx, endpoint.c_str(), 0, array, resp))
		handleSoapFault("Failed to list requests: listRequests.");
}

void GSoapContextAdapter::listVoManagers(string vo, impltns__listVOManagersResponse& resp) {
	if (soap_call_impltns__listVOManagers(ctx, endpoint.c_str(), 0, vo, resp))
		handleSoapFault("Failed to list VO managers: listVOManagers.");
}

void GSoapContextAdapter::getTransferJobSummary2 (string jobId, impltns__getTransferJobSummary2Response& resp) {
	if (soap_call_impltns__getTransferJobSummary2(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobSummary2.");
}

void GSoapContextAdapter::getTransferJobSummary (string jobId, impltns__getTransferJobSummaryResponse& resp) {
	if (soap_call_impltns__getTransferJobSummary(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobSummary.");
}

void GSoapContextAdapter::getFileStatus (string jobId, impltns__getFileStatusResponse& resp) {
	if (soap_call_impltns__getFileStatus(ctx, endpoint.c_str(), 0, jobId, 0, 100, resp))
		handleSoapFault("Failed to get job status: getFileStatus.");
}

void GSoapContextAdapter::setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp) {
	if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
		handleSoapFault("Failed to set configuration: setConfiguration.");
}

void GSoapContextAdapter::getConfiguration (implcfg__getConfigurationResponse& resp) {
	if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, resp))
		handleSoapFault("Failed to get configuration: getConfiguration.");
}


void GSoapContextAdapter::handleSoapFault(string msg) {
	cout << "gsoap fault: ";
	soap_stream_fault(ctx, cout);
	throw string(msg);
}

void GSoapContextAdapter::setInterfaceVersion(string interface) {

	if (interface.empty()) return;

	// set the seperator that will be used for tokenizing
	char_separator<char> sep(".");
	tokenizer< char_separator<char> > tokens(interface, sep);
	tokenizer< char_separator<char> >::iterator it = tokens.begin();

	if (it == tokens.end()) return;

	string s = *it++;
	major = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it++;
	minor = lexical_cast<long>(s);

	if (it == tokens.end()) return;

	s = *it;
	patch = lexical_cast<long>(s);
}


int GSoapContextAdapter::isChecksumSupported() {
    return isItVersion370();
}

int GSoapContextAdapter::isDelegationSupported() {
    return isItVersion330();
}

int GSoapContextAdapter::isRolesOfSupported() {
    return isItVersion330();
}

int GSoapContextAdapter::isSetTCPBufferSupported() {
    return isItVersion330();
}

int GSoapContextAdapter::isUserVoRestrictListingSupported() {
    return isItVersion330();
}

int GSoapContextAdapter::isVersion330StatesSupported(){
    return isItVersion330();
}

int GSoapContextAdapter::isItVersion330() {
    return major >= 3 && minor >= 3 && patch >= 0;
}

int GSoapContextAdapter::isItVersion340() {
    return major >= 3 && minor >= 4 && patch >= 0;
}

int GSoapContextAdapter::isItVersion350() {
    return major >= 3 && minor >= 5 && patch >= 0;
}

int GSoapContextAdapter::isItVersion370() {
	return major >= 3 && minor >= 7 && patch >= 0;
}

