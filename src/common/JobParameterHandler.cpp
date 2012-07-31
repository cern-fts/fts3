/*
 * JobParameterHandler.cpp
 *
 *  Created on: Mar 12, 2012
 *      Author: simonm
 */

#include "JobParameterHandler.h"

using namespace fts3::common;

const string JobParameterHandler::FTS3_PARAM_GRIDFTP = "gridftp";
const string JobParameterHandler::FTS3_PARAM_MYPROXY = "myproxy";
const string JobParameterHandler::FTS3_PARAM_DELEGATIONID = "delegationid";
const string JobParameterHandler::FTS3_PARAM_SPACETOKEN = "spacetoken";
const string JobParameterHandler::FTS3_PARAM_SPACETOKEN_SOURCE = "source_spacetoken";
const string JobParameterHandler::FTS3_PARAM_COPY_PIN_LIFETIME = "copy_pin_lifetime";
const string JobParameterHandler::FTS3_PARAM_LAN_CONNECTION = "lan_connection";
const string JobParameterHandler::FTS3_PARAM_FAIL_NEARLINE = "fail_nearline";
const string JobParameterHandler::FTS3_PARAM_OVERWRITEFLAG = "overwrite";
const string JobParameterHandler::FTS3_PARAM_CHECKSUM_METHOD = "checksum_method";
const string JobParameterHandler::FTS3_PARAM_REUSE = "reuse";


JobParameterHandler::JobParameterHandler() {
	// set defaults
	parameters[FTS3_PARAM_COPY_PIN_LIFETIME] = "1";
}


JobParameterHandler::~JobParameterHandler() {

}

void JobParameterHandler::operator() (vector<string>& keys, vector<string>& values) {

	// set the given parameters
	for (vector<string>::iterator it_keys = keys.begin(), it_val = values.begin(); it_keys < keys.end(); it_keys++, it_val++) {
		parameters[*it_keys] = *it_val;
	}
}

