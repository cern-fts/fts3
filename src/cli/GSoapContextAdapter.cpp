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
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include <cgsi_plugin.h>

#include <sstream>
#include <fstream>

namespace fts3 { namespace cli {

GSoapContextAdapter::GSoapContextAdapter(string endpoint): endpoint(endpoint), ctx(soap_new()/*soap_new1(SOAP_ENC_MTOM)*/) {

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
    	err = soap_cgsi_init(ctx, CGSI_OPT_DISABLE_NAME_CHECK );
    }
//
//    soap_set_omode(ctx, SOAP_ENC_MTOM);
//
//	ctx->fmimewriteopen = LogFileStreamer::writeOpen;
//	ctx->fmimewriteclose = LogFileStreamer::writeClose;
//	ctx->fmimewrite = LogFileStreamer::write;

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

string GSoapContextAdapter::transferSubmit (vector<JobElement> elements, map<string, string> parameters) {

	if (elements.empty()) {
    	throw string ("No transfer job has been specified.");
	}

	// the transfer job
	tns3__TransferJob3 job;

	// the job's elements
	tns3__TransferJobElement3* element;

	// iterate over the internal vector containing job elements
	vector<JobElement>::iterator e_it;
	for (e_it = elements.begin(); e_it < elements.end(); e_it++) {

		const string& source = get<SOURCE>(*e_it);
		const string& destination = get<DESTINATION>(*e_it);
		const optional<string>& checksum = get<CHECKSUM>(*e_it);
		const optional<int>& filesize = get<FILE_SIZE>(*e_it);
		const optional<string>& metadata = get<FILE_METADATA>(*e_it);

		// create the job element, and set the source, destination and checksum values
		element = soap_new_tns3__TransferJobElement3(ctx, -1);

		// set the required fields (source and destination)
		element->source = source;
		element->dest = destination;

		// set the optional fields:

		// the checksum
		if (checksum) {
			element->checksum = soap_new_std__string(ctx, -1);
			// check checksum format
			string checksum_str = *checksum;
			string::size_type colon = checksum_str.find(":");
			if (colon == string::npos || colon == 0 || colon == checksum_str.size() - 1) {
				throw string("Checksum format is not valid (ALGORITHM:1234af).");
			}
			*element->checksum = checksum_str;
		} else {
			element->checksum = 0;
		}

		// the file size
		if (filesize) {
			element->filesize = (int*) soap_malloc(ctx, sizeof(int));
			*element->filesize = *filesize;
		} else {
			element->filesize = 0;
		}

		// the file metadata
		if (metadata) {
			element->metadata = soap_new_std__string(ctx, -1);
			*element->metadata = *metadata;
		} else {
			element->metadata = 0;
		}

		// push the element into the result vector
		job.transferJobElements.push_back(element);
	}


	job.jobParams = soap_new_tns3__TransferParams(ctx, -1);
	map<string, string>::iterator p_it;

	for (p_it = parameters.begin(); p_it != parameters.end(); p_it++) {
		job.jobParams->keys.push_back(p_it->first);
		job.jobParams->values.push_back(p_it->second);
	}

	impltns__transferSubmit4Response resp;
	if (soap_call_impltns__transferSubmit4(ctx, endpoint.c_str(), 0, &job, resp))
		handleSoapFault("Failed to submit transfer: transferSubmit3.");

	return resp._transferSubmit4Return;
}

string GSoapContextAdapter::transferSubmit (vector<JobElement> elements, map<string, string> parameters, string password) {

	if (elements.empty()) {
    	throw string ("No transfer job has been specified.");
	}

	// the transfer job
	tns3__TransferJob job;

	// set the credential
	job.credential = soap_new_std__string(ctx, -1);
	*job.credential = password;

	tns3__TransferJobElement* element;
	vector<JobElement>::iterator it_e;

	// iterate over the internal vector containing Task (job elements)
	for (it_e = elements.begin(); it_e < elements.end(); it_e++) {

		// create the job element, and set the source, destination and checksum values
		element = soap_new_tns3__TransferJobElement(ctx, -1);
		element->source = soap_new_std__string(ctx, -1);
		element->dest = soap_new_std__string(ctx, -1);

		*element->source = get<SOURCE>(*it_e);
		*element->dest = get<DESTINATION>(*it_e);

		// push the element into the result vector
		job.transferJobElements.push_back(element);
	}

	job.jobParams = soap_new_tns3__TransferParams(ctx, -1);
	map<string, string>::iterator it_p;

	for (it_p = parameters.begin(); it_p != parameters.end(); it_p++) {
		job.jobParams->keys.push_back(it_p->first);
		job.jobParams->values.push_back(it_p->second);
	}

	impltns__transferSubmitResponse resp;
	if (soap_call_impltns__transferSubmit(ctx, endpoint.c_str(), 0, &job, resp))
		handleSoapFault("Failed to submit transfer: transferSubmit.");

	return resp._transferSubmitReturn;
}

JobStatus GSoapContextAdapter::getTransferJobStatus (string jobId) {

	impltns__getTransferJobStatusResponse resp;
	if (soap_call_impltns__getTransferJobStatus(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobStatus.");

	if (!resp._getTransferJobStatusReturn)
		throw string("The response from the server is empty!");

	return JobStatus(
			*resp._getTransferJobStatusReturn->jobID,
			*resp._getTransferJobStatusReturn->jobStatus,
			*resp._getTransferJobStatusReturn->clientDN,
			*resp._getTransferJobStatusReturn->reason,
			*resp._getTransferJobStatusReturn->voName,
			resp._getTransferJobStatusReturn->submitTime,
			resp._getTransferJobStatusReturn->numFiles,
			resp._getTransferJobStatusReturn->priority
		);
}

void GSoapContextAdapter::cancel(vector<string> jobIds) {

	impltns__ArrayOf_USCOREsoapenc_USCOREstring rqst;
	rqst.item = jobIds;

	impltns__cancelResponse resp;
	if (soap_call_impltns__cancel(ctx, endpoint.c_str(), 0, &rqst, resp))
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

vector<JobStatus> GSoapContextAdapter::listRequests2 (vector<string> statuses, string dn, string vo) {

	impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
	array->item = statuses;

	impltns__listRequests2Response resp;
	if (soap_call_impltns__listRequests2(ctx, endpoint.c_str(), 0, array, dn, vo, resp))
		handleSoapFault("Failed to list requests: listRequests2.");

	if (!resp._listRequests2Return)
		throw string("The response from the server is empty!");

	vector<JobStatus> ret;
	vector<tns3__JobStatus*>::iterator it;

	for (it = resp._listRequests2Return->item.begin(); it < resp._listRequests2Return->item.end(); it++) {
		tns3__JobStatus* gstat = *it;

		JobStatus status = JobStatus(
				*gstat->jobID,
				*gstat->jobStatus,
				*gstat->clientDN,
				*gstat->reason,
				*gstat->voName,
				gstat->submitTime,
				gstat->numFiles,
				gstat->priority
			);
		ret.push_back(status);
	}

	return ret;
}

vector<JobStatus> GSoapContextAdapter::listRequests (vector<string> statuses) {

	impltns__ArrayOf_USCOREsoapenc_USCOREstring* array = soap_new_impltns__ArrayOf_USCOREsoapenc_USCOREstring(ctx, -1);
	array->item = statuses;

	impltns__listRequestsResponse resp;
	if (soap_call_impltns__listRequests(ctx, endpoint.c_str(), 0, array, resp))
		handleSoapFault("Failed to list requests: listRequests.");

	if (!resp._listRequestsReturn)
		throw string("The response from the server is empty!");

	vector<JobStatus> ret;
	vector<tns3__JobStatus*>::iterator it;

	for (it = resp._listRequestsReturn->item.begin(); it < resp._listRequestsReturn->item.end(); it++) {
		tns3__JobStatus* gstat = *it;

		JobStatus status = JobStatus(
				*gstat->jobID,
				*gstat->jobStatus,
				*gstat->clientDN,
				*gstat->reason,
				*gstat->voName,
				gstat->submitTime,
				gstat->numFiles,
				gstat->priority
			);
		ret.push_back(status);
	}

	return ret;
}

void GSoapContextAdapter::listVoManagers(string vo, impltns__listVOManagersResponse& resp) {
	if (soap_call_impltns__listVOManagers(ctx, endpoint.c_str(), 0, vo, resp))
		handleSoapFault("Failed to list VO managers: listVOManagers.");
}

JobSummary GSoapContextAdapter::getTransferJobSummary2 (string jobId) {

	impltns__getTransferJobSummary2Response resp;
	if (soap_call_impltns__getTransferJobSummary2(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobSummary2.");

	if (!resp._getTransferJobSummary2Return)
		throw string("The response from the server is empty!");

	JobStatus status = JobStatus(
			*resp._getTransferJobSummary2Return->jobStatus->jobID,
			*resp._getTransferJobSummary2Return->jobStatus->jobStatus,
			*resp._getTransferJobSummary2Return->jobStatus->clientDN,
			*resp._getTransferJobSummary2Return->jobStatus->reason,
			*resp._getTransferJobSummary2Return->jobStatus->voName,
			resp._getTransferJobSummary2Return->jobStatus->submitTime,
			resp._getTransferJobSummary2Return->jobStatus->numFiles,
			resp._getTransferJobSummary2Return->jobStatus->priority
		);

	return JobSummary (
			status,
			resp._getTransferJobSummary2Return->numActive,
			resp._getTransferJobSummary2Return->numCanceled,
			resp._getTransferJobSummary2Return->numFailed,
			resp._getTransferJobSummary2Return->numFinished,
			resp._getTransferJobSummary2Return->numSubmitted,
			resp._getTransferJobSummary2Return->numReady
		);
}

JobSummary GSoapContextAdapter::getTransferJobSummary (string jobId) {

	impltns__getTransferJobSummaryResponse resp;
	if (soap_call_impltns__getTransferJobSummary(ctx, endpoint.c_str(), 0, jobId, resp))
		handleSoapFault("Failed to get job status: getTransferJobSummary.");

	if (!resp._getTransferJobSummaryReturn)
		throw string("The response from the server is empty!");

	JobStatus status = JobStatus(
			*resp._getTransferJobSummaryReturn->jobStatus->jobID,
			*resp._getTransferJobSummaryReturn->jobStatus->jobStatus,
			*resp._getTransferJobSummaryReturn->jobStatus->clientDN,
			*resp._getTransferJobSummaryReturn->jobStatus->reason,
			*resp._getTransferJobSummaryReturn->jobStatus->voName,
			resp._getTransferJobSummaryReturn->jobStatus->submitTime,
			resp._getTransferJobSummaryReturn->jobStatus->numFiles,
			resp._getTransferJobSummaryReturn->jobStatus->priority
		);

	return JobSummary (
			status,
			resp._getTransferJobSummaryReturn->numActive,
			resp._getTransferJobSummaryReturn->numCanceled,
			resp._getTransferJobSummaryReturn->numFailed,
			resp._getTransferJobSummaryReturn->numFinished,
			resp._getTransferJobSummaryReturn->numSubmitted,
			0 // we don't care that much about this one, hope to remove this version of getTransferJobSummary TODO
		);
}

void GSoapContextAdapter::getFileStatus (string jobId, impltns__getFileStatusResponse& resp) {
	if (soap_call_impltns__getFileStatus(ctx, endpoint.c_str(), 0, jobId, 0, 100, resp))
		handleSoapFault("Failed to get job status: getFileStatus.");
}

void GSoapContextAdapter::setConfiguration (config__Configuration *config, implcfg__setConfigurationResponse& resp) {
	if (soap_call_implcfg__setConfiguration(ctx, endpoint.c_str(), 0, config, resp))
		handleSoapFault("Failed to set configuration: setConfiguration.");
}

void GSoapContextAdapter::getConfiguration (string src, string dest, string vo, string name, implcfg__getConfigurationResponse& resp) {
	if (soap_call_implcfg__getConfiguration(ctx, endpoint.c_str(), 0, vo, name, src, dest, resp))
		handleSoapFault("Failed to get configuration: getConfiguration.");
}

void GSoapContextAdapter::delConfiguration(config__Configuration *config, implcfg__delConfigurationResponse& resp) {
	if (soap_call_implcfg__delConfiguration(ctx, endpoint.c_str(), 0, config, resp))
		handleSoapFault("Failed to delete configuration: delConfiguration.");
}

void GSoapContextAdapter::debugSet(string source, string destination, bool debug) {
	impltns__debugSetResponse resp;
	if (soap_call_impltns__debugSet(ctx, endpoint.c_str(), 0, source, destination, debug, resp)) {
		handleSoapFault("Operation debugSet failed.");
	}
}

void GSoapContextAdapter::blacklist(string type, string subject, bool mode) {
	impltns__blacklistResponse resp;
	if (soap_call_impltns__blacklist(ctx, endpoint.c_str(), 0, type, subject, mode, resp)) {
		handleSoapFault("Operation blacklist failed.");
	}
}

void GSoapContextAdapter::doDrain(bool drain) {
	implcfg__doDrainResponse resp;
	if (soap_call_implcfg__doDrain(ctx, endpoint.c_str(), 0, drain, resp)) {
		handleSoapFault("Operation doDrain failed.");
	}
}

void GSoapContextAdapter::prioritySet(string jobId, int priority) {
	impltns__prioritySetResponse resp;
	if (soap_call_impltns__prioritySet(ctx, endpoint.c_str(), 0, jobId, priority, resp)) {
		handleSoapFault("Operation prioritySet failed.");
	}
}

void GSoapContextAdapter::retrySet(int retry) {
	implcfg__setRetryResponse resp;
	if (soap_call_implcfg__setRetry(ctx, endpoint.c_str(), 0, retry, resp)) {
		handleSoapFault("Operation retrySet failed.");
	}
}

void GSoapContextAdapter::queueTimeoutSet(unsigned timeout) {
	implcfg__setQueueTimeoutResponse resp;
	if (soap_call_implcfg__setQueueTimeout(ctx, endpoint.c_str(), 0, timeout, resp)) {
		handleSoapFault("Operation queueTimeoutSet failed.");
	}
}

void GSoapContextAdapter::getLog(string& logname, string jobId) {

	log__GetLogResponse resp;
	if (soap_call_log__GetLog(ctx, endpoint.c_str(), 0, jobId, resp)) {
		handleSoapFault("Operation getLog failed.");
		return;
	}

	fstream file (logname.c_str(), ios::out);
	file.write((char*) resp.log->xop__Include.__ptr, resp.log->xop__Include.__size);
	file.flush();
	file.close();
}

void GSoapContextAdapter::handleSoapFault(string msg) {

	stringstream ss;
	soap_stream_fault(ctx, ss);

	// replace the standard gSOAP error message before printing
	msg = ss.str();

	regex re (".+CGSI-gSOAP running on .+ reports Error reading token data header: Connection closed.+");
	if (regex_match(ss.str(), re)) {
		msg += " It might be the FTS server's CRL has expired!";
	}

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

}
}
