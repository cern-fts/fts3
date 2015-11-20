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

#include "JobStatusHandler.h"
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

#include <set>


using namespace boost::assign;
using namespace fts3::common;
using namespace boost;

// Initialise std::string constants
const std::string JobStatusHandler::FTS3_STATUS_FINISHEDDIRTY = "FINISHEDDIRTY";
const std::string JobStatusHandler::FTS3_STATUS_CANCELED = "CANCELED";
const std::string JobStatusHandler::FTS3_STATUS_UNKNOWN = "UNKNOWN";
const std::string JobStatusHandler::FTS3_STATUS_FAILED = "FAILED";
const std::string JobStatusHandler::FTS3_STATUS_FINISHED = "FINISHED";
const std::string JobStatusHandler::FTS3_STATUS_SUBMITTED = "SUBMITTED";
const std::string JobStatusHandler::FTS3_STATUS_READY = "READY";
const std::string JobStatusHandler::FTS3_STATUS_ACTIVE = "ACTIVE";
const std::string JobStatusHandler::FTS3_STATUS_STAGING = "STAGING";
const std::string JobStatusHandler::FTS3_STATUS_NOT_USED = "NOT_USED";
const std::string JobStatusHandler::FTS3_STATUS_DELETE = "DELETE";
const std::string JobStatusHandler::FTS3_STATUS_STARTED = "STARTED";

JobStatusHandler::JobStatusHandler():
    statusNameToId(map_list_of
                   (FTS3_STATUS_FINISHEDDIRTY, FTS3_STATUS_FINISHEDDIRTY_ID)
                   (FTS3_STATUS_CANCELED, FTS3_STATUS_CANCELED_ID)
                   (FTS3_STATUS_UNKNOWN, FTS3_STATUS_UNKNOWN_ID)
                   (FTS3_STATUS_SUBMITTED, FTS3_STATUS_SUBMITTED_ID)
                   (FTS3_STATUS_ACTIVE, FTS3_STATUS_ACTIVE_ID)
                   (FTS3_STATUS_FINISHED, FTS3_STATUS_FINISHED_ID)
                   (FTS3_STATUS_READY, FTS3_STATUS_READY_ID)
                   (FTS3_STATUS_FAILED, FTS3_STATUS_FAILED_ID)
                   (FTS3_STATUS_STAGING, FTS3_STATUS_STAGING_ID)
                   (FTS3_STATUS_NOT_USED, FTS3_STATUS_NOT_USED_ID)
                   (FTS3_STATUS_DELETE, FTS3_STATUS_DELETE_ID)
                   (FTS3_STATUS_STARTED, FTS3_STATUS_STARTED_ID).to_container(statusNameToId))
{
    // the constant map is initialised in initialiser list
}


bool JobStatusHandler::isTransferFinished(std::string status)
{
    to_upper(status);
    std::map<std::string, JobStatusEnum>::const_iterator it = statusNameToId.find(status);

    if (it == statusNameToId.end()) {
        return FTS3_STATUS_UNKNOWN_ID <= 0;
    }
    return it->second <= 0;
}


bool JobStatusHandler::isStatusValid(std::string status)
{
    to_upper(status);
    return statusNameToId.find(status) != statusNameToId.end();
}


size_t JobStatusHandler::countInState(const std::string &status, const std::vector<FileTransferStatus> &statuses)
{
    std::set<int> files;

    for (auto it = statuses.begin(); it < statuses.end(); it++) {
        if (status == it->fileState) {
            files.insert(it->fileIndex);
        }
    }

    return files.size();
}
