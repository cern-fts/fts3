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

#include <algorithm>
#include <iterator>

#include <boost/iterator/zip_iterator.hpp>
#include <boost/tuple/tuple.hpp>

using namespace fts3::common;

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

void JobParameterHandler::operator() (std::vector<std::string>& keys, std::vector<std::string>& values)
{
    std::transform(
        boost::make_zip_iterator(boost::make_tuple(keys.begin(), values.begin())),
        boost::make_zip_iterator(boost::make_tuple(keys.end(), values.end())),
        std::inserter(parameters, parameters.begin()),
        zipper()
    );
}

