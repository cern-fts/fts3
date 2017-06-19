/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JobParameterHandler.h"
#include <cassert>

using namespace fts3::cli;

const std::string JobParameterHandler::GRIDFTP = "gridftp";
const std::string JobParameterHandler::DELEGATIONID = "delegationid";
const std::string JobParameterHandler::SPACETOKEN = "spacetoken";
const std::string JobParameterHandler::SPACETOKEN_SOURCE = "source_spacetoken";
const std::string JobParameterHandler::COPY_PIN_LIFETIME = "copy_pin_lifetime";
const std::string JobParameterHandler::BRING_ONLINE = "bring_online";
const std::string JobParameterHandler::LAN_CONNECTION = "lan_connection";
const std::string JobParameterHandler::FAIL_NEARLINE = "fail_nearline";
const std::string JobParameterHandler::OVERWRITEFLAG = "overwrite";
const std::string JobParameterHandler::CHECKSUM_METHOD = "checksum_method";
const std::string JobParameterHandler::CHECKSUM_MODE = "checksum_mode";
const std::string JobParameterHandler::REUSE = "reuse";
const std::string JobParameterHandler::JOB_METADATA = "job_metadata";
const std::string JobParameterHandler::RETRY = "retry";
const std::string JobParameterHandler::RETRY_DELAY = "retry_delay";
const std::string JobParameterHandler::MULTIHOP = "multihop";
const std::string JobParameterHandler::BUFFER_SIZE = "buffer_size";
const std::string JobParameterHandler::NOSTREAMS = "nostreams";
const std::string JobParameterHandler::TIMEOUT = "timeout";
const std::string JobParameterHandler::STRICT_COPY = "strict_copy";
const std::string JobParameterHandler::CREDENTIALS = "credentials";


JobParameterHandler::JobParameterHandler()
{

}


JobParameterHandler::~JobParameterHandler()
{

}


void JobParameterHandler::set(const std::vector<std::string>& keys, const std::vector<std::string>& values)
{
    assert(keys.size() == values.size());

    for (size_t i = 0; i < keys.size(); ++i) {
        parameters[keys[i]] = values[i];
    }
}
