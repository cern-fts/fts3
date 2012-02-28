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

using namespace fts::ws;
using namespace boost;
using namespace db;
using namespace fts3::config;

/// Web service operation 'transferSubmit' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit(transfer__TransferJob *_job, struct fts__transferSubmitResponse &_param_3) {

	if (_job == 0) {
		transfer__InvalidArgumentException ex;
		throw ex;
	}

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

	try{
	    const string requestID = id;
	    const string dn ="/C=DE/O=GermanGrid/OU=DESY/CN=galway.desy.de";
	    const string vo = string("dteam");

	    vector<src_dest_checksum_tupple> jobs;
	    vector<transfer__TransferJobElement * >::iterator it;

	    for (it = _job->transferJobElements.begin(); it < _job->transferJobElements.end(); it++) {
	    	src_dest_checksum_tupple tupple;
	    	tupple.source = *(*it)->source;
	    	tupple.destination = *(*it)->dest;
	    	jobs.push_back(tupple);
	    }

	    string sourceSpaceTokenDescription = "";

	    string cred = "";
	    if (_job->credential) {
	    	cred = *_job->credential;
	    }
	    int copyPinLifeTime = 1;


	    vector<string> params = FtsServiceTask::getParams(_job, copyPinLifeTime);

	    /*string params[9];

	    if(_job->jobParams) {

	    	vector<string>::iterator key_it = _job->jobParams->keys.begin();
	    	vector<string>::iterator val_it = _job->jobParams->values.begin();

	    	map<string, int> index;
	    	index.insert(pair<string, int>("gridftp", 0));
	    	index.insert(pair<string, int>("myproxy", 1));
	    	index.insert(pair<string, int>("delegationid", 2));
	    	index.insert(pair<string, int>("spacetoken", 3));
	    	index.insert(pair<string, int>("overwrite", 4));
	    	index.insert(pair<string, int>("source_spacetoken", 5));
	    	index.insert(pair<string, int>("lan_connection", 6));
	    	index.insert(pair<string, int>("fail_nearline", 7));
	    	index.insert(pair<string, int>("checksum_method", 8));

	    	map<string, int>::iterator index_it;


	    	for (; key_it < _job->jobParams->keys.end(); key_it++, val_it++) {
	    		if (key_it->compare("copy_pin_lifetime") == 0) {
	    			copyPinLifeTime = lexical_cast<int>(*val_it);
	    		} else {
	    			index_it = index.find(*key_it);

	    			if (index_it != index.end()) {
	    				params[index_it->second] = *val_it;
	    			}
	    		}
	    	}
	    }*/

	    DBSingleton::instance().getDBObjectInstance()->submitPhysical(requestID, jobs, params[0],
	                                 dn, cred, vo, params[1],
	                                 params[2], params[3], params[4],
	                                 params[5], sourceSpaceTokenDescription, params[6], copyPinLifeTime,
	                                 params[7], params[8]);
	  }
	catch (string const &e)
	  {
	    cout << e << endl;
	  }

	_param_4._transferSubmit2Return = id;
	return SOAP_OK;
}

/// Web service operation 'transferSubmit3' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::transferSubmit3(transfer__TransferJob2 *_job, struct fts__transferSubmit3Response &_param_5) {


	// check sum !

	if (_job == 0) {
		transfer__InvalidArgumentException ex;
		throw ex;
	}

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
int FileTransferSoapBindingService::getFileStatus(string _requestID, int _offset, int _limit, struct fts__getFileStatusResponse &_param_9) {
	return SOAP_OK;
}

/// Web service operation 'getFileStatus2' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getFileStatus2(string _requestID, int _offset, int _limit, struct fts__getFileStatus2Response &_param_10) {
	return SOAP_OK;
}

/// Web service operation 'getTransferJobStatus' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getTransferJobStatus(string _requestID, struct fts__getTransferJobStatusResponse &_param_11) {

	try{
		JobStatus* record =  DBSingleton::instance().getDBObjectInstance()->getTransferJobStatus(_requestID);

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

			delete record;
		}
	} catch (string const &e) {
		cout << e << endl;
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
	_param_21.getVersionReturn = "0.0.1-1";
	return SOAP_OK;
}

/// Web service operation 'getSchemaVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getSchemaVersion(struct fts__getSchemaVersionResponse &_param_22) {
	_param_22.getSchemaVersionReturn = "0.0.1";
	return SOAP_OK;
}

/// Web service operation 'getInterfaceVersion' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getInterfaceVersion(struct fts__getInterfaceVersionResponse &_param_23) {
	_param_23.getInterfaceVersionReturn = "0.0.1";
	return SOAP_OK;
}

/// Web service operation 'getServiceMetadata' (returns error code or SOAP_OK)
int FileTransferSoapBindingService::getServiceMetadata(string _key, struct fts__getServiceMetadataResponse &_param_24) {
	_param_24._getServiceMetadataReturn = "fts3-data-transfer-service-0.0.1-1";
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
