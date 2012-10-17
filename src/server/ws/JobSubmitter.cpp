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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * JobSubmitter.cpp
 *
 *  Created on: Mar 7, 2012
 *      Author: Michal Simon
 */

#include "JobSubmitter.h"
#include "uuid_generator.h"
#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"
#include "CfgBlocks.h"

#include "GSoapDelegationHandler.h"

#include <boost/lexical_cast.hpp>
#include <algorithm>


using namespace boost;
using namespace db;
using namespace fts3::ws;

JobSubmitter::JobSubmitter(soap* soap, tns3__TransferJob *job, bool delegation) {

	GSoapDelegationHandler handler (soap);
	delegationId = handler.makeDelegationId();
	vo = handler.getClientVo();
	dn = handler.getClientDn();

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is submitting a transfer job" << commit;

	// check weather the job is well specified
	if (job == 0 || job->transferJobElements.empty()) {
		throw Err_Custom("The job was not defined");
	}

	// do the common initialization
	init(job->jobParams);

	// check the delegation and MyProxy password settings
	if (delegation) {
		if (job->credential) {
			throw Err_Custom("The MyProxy password should not be provided if delegation is used");
		}
	} else {
		if (params.isParamSet(JobParameterHandler::FTS3_PARAM_DELEGATIONID)) {
			throw Err_Custom("The delegation ID should not be provided if MyProxy password mode is used");
		}

		if (!job->credential || job->credential->empty()) {
			throw Err_Custom("The MyProxy password is empty while submitting in MyProxy mode");
		}

		cred = *job->credential;
	}

	if (!job->transferJobElements.empty()) {
		string* src = (*job->transferJobElements.begin())->source;
		string* dest = (*job->transferJobElements.begin())->dest;

		sourceSe = CfgBlocks::fileUrlToSeName(*src).get();
		destinationSe = CfgBlocks::fileUrlToSeName(*dest).get();
		if(sourceSe.length()==0){
			std::string errMsg = "Can't extract hostname from url " + *src;
			throw Err_Custom(errMsg);
		}
		
		if(destinationSe.length()==0){
			std::string errMsg = "Can't extract hostname from url " + *dest;		
			throw Err_Custom(errMsg);
		}
	}

	// extract the job elements from tns3__TransferJob object and put them into a vector
    vector<tns3__TransferJobElement * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); it++) {

    	string src = *(*it)->source, dest = *(*it)->dest;
    	// check weather the source and destination files are supported
    	if (!checkProtocol(dest)) throw Err_Custom("Destination protocol not supported (" + dest + ")");
    	if (!checkProtocol(src) && !checkIfLfn(src)) throw Err_Custom("Source protocol not supported (" + src + ")");

    	src_dest_checksum_tupple tupple;
    	tupple.source = src;
    	tupple.destination = dest;
        
    	jobs.push_back(tupple);
    }
    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;
}

JobSubmitter::JobSubmitter(soap* soap, tns3__TransferJob2 *job) {

	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Constructing JobSubmitter" << commit;

	GSoapDelegationHandler handler (soap);
	delegationId = handler.makeDelegationId();
	vo = handler.getClientVo();
        dn = handler.getClientDn();

	// check weather the job is well specified
	if (job == 0 || job->transferJobElements.empty()) {
		throw Err_Custom("The job was not defined");
	}

	// checksum uses always delegation?
	if (job->credential) {
		throw Err_Custom("The MyProxy password should not be provided if delegation is used");
    }

	// do the common initialization
	init(job->jobParams);

        if (!job->transferJobElements.empty()) {
                string* src = (*job->transferJobElements.begin())->source;
                string* dest = (*job->transferJobElements.begin())->dest;

                sourceSe = CfgBlocks::fileUrlToSeName(*src).get();
                destinationSe = CfgBlocks::fileUrlToSeName(*dest).get();
        }


	// extract the job elements from tns3__TransferJob2 object and put them into a vector
    vector<tns3__TransferJobElement2 * >::iterator it;
    for (it = job->transferJobElements.begin(); it < job->transferJobElements.end(); it++) {

    	string src = *(*it)->source, dest = *(*it)->dest;

    	// check weather the destination file is supported
    	if (!checkProtocol(dest)) {
    		throw Err_Custom("Destination protocol is not supported for file: " + dest);
    	}
    	// check weather the source file is supported
    	if (!checkProtocol(src) && !checkIfLfn(src)) {
    		throw Err_Custom("Source protocol is not supported for file: " + src);
    	}

    	src_dest_checksum_tupple tupple;
    	tupple.source = src;
    	tupple.destination = dest;
    	tupple.checksum = *(*it)->checksum;
        
    	jobs.push_back(tupple);
    }
    FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Job's vector has been created" << commit;
}

void JobSubmitter::init(tns3__TransferParams *jobParams) {

	id = UuidGenerator::generateUUID();
	FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Generated uuid " << id << commit;

    sourceSpaceTokenDescription = "";
    copyPinLifeTime = 1;

    if (jobParams) {
    	params(jobParams->keys, jobParams->values);
        FTS3_COMMON_LOGGER_NEWLOG (DEBUG) << "Parameter map has been created" << commit;
    }
}

JobSubmitter::~JobSubmitter() {

}

string JobSubmitter::submit() {
    
    
    DBSingleton::instance().getDBObjectInstance()->submitPhysical (
    		id,
    		jobs,
    		params.get(JobParameterHandler::FTS3_PARAM_GRIDFTP),
            dn,
            cred,
            vo,
            params.get(JobParameterHandler::FTS3_PARAM_MYPROXY),
            delegationId,
            params.get(JobParameterHandler::FTS3_PARAM_SPACETOKEN),
            params.get(JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG),
            params.get(JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE),
            sourceSpaceTokenDescription,
            params.get(JobParameterHandler::FTS3_PARAM_LAN_CONNECTION),
            params.get<int>(JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME),
            params.get(JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE),
            params.get(JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD),
            params.get(JobParameterHandler::FTS3_PARAM_REUSE),
            sourceSe,
            destinationSe
    	);


    FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The jobid " << id << " has been submitted successfully" << commit;
	return id;
}

bool JobSubmitter::checkProtocol(string file) {
	string tmp (file);
	transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);

	return tmp.find("srm://") == 0 || tmp.find("gsiftp://") == 0;
}

bool JobSubmitter::checkIfLfn(string file) {
	return file.find("/") == 0 && file.find(";") == string::npos && file.find(":") == string::npos;
}
