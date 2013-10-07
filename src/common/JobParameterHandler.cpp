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
 *
 *
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
const string JobParameterHandler::RETRY = "retry";
const string JobParameterHandler::RETRY_DELAY = "retry_delay";
const string JobParameterHandler::MULTIHOP = "multihop";

JobParameterHandler::JobParameterHandler()
{

}


JobParameterHandler::~JobParameterHandler()
{

}

void JobParameterHandler::operator() (vector<string>& keys, vector<string>& values)
{

    // set the given parameters
    for (vector<string>::iterator it_keys = keys.begin(), it_val = values.begin(); it_keys < keys.end(); it_keys++, it_val++)
        {
            parameters[*it_keys] = *it_val;
        }
}

