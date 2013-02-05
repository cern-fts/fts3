/*
 * JobParameterHandler.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: simonm
 */

#include "JobParameterHandler.h"

using namespace fts3::common;

const string JobParameterHandler::GRIDFTP = "gridftp";
const string JobParameterHandler::DELEGATIONID = "delegationid";
const string JobParameterHandler::SPACETOKEN = "spacetoken";
const string JobParameterHandler::SPACETOKEN_SOURCE = "source_spacetoken";
const string JobParameterHandler::COPY_PIN_LIFETIME = "copy_pin_lifetime";
const string JobParameterHandler::BRING_ONLINE = "bring_online";
const string JobParameterHandler::LAN_CONNECTION = "lan_connection";
const string JobParameterHandler::FAIL_NEARLINE = "fail_nearline";
const string JobParameterHandler::OVERWRITEFLAG = "overwrite";
const string JobParameterHandler::CHECKSUM_METHOD = "checksum_method";
const string JobParameterHandler::REUSE = "reuse";
const string JobParameterHandler::JOB_METADATA = "job_metadata";


JobParameterHandler::JobParameterHandler() {

}


JobParameterHandler::~JobParameterHandler() {

}

void JobParameterHandler::operator() (vector<string>& keys, vector<string>& values) {

	// set the given parameters
	for (vector<string>::iterator it_keys = keys.begin(), it_val = values.begin(); it_keys < keys.end(); it_keys++, it_val++) {
		parameters[*it_keys] = *it_val;
	}
}

