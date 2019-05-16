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

#include "StagingContext.h"

#include <sstream>
#include <unordered_set>
#include "cred/CredUtility.h"


void ArchivingContext::add(const ArchivingOperation &stagingOp)
{


    if (stagingOp.timeout > bringonlineTimeout) {
        bringonlineTimeout = stagingOp.timeout;
    }

    add(stagingOp.surl, stagingOp.jobId, stagingOp.fileId);
}


bool ArchivingContext::hasTimeoutExpired()
{
    return difftime(time(0), startTime) > bringonlineTimeout;
}


std::set<std::string> ArchivingContext::getSurlsToAbort(
    const std::set<std::pair<std::string, std::string>> &urls)
{
    // remove respective URLs from the task
    for (auto it = urls.begin(); it != urls.end(); ++it) {
        jobs[it->first].erase(it->second);
    }

    std::unordered_set<std::string> not_canceled, unique;
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j) {
        auto const & urls = it_j->second;
        for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u) {
            std::string const & url = it_u->first;
            not_canceled.insert(url);
        }
    }

    std::set<std::string> ret;
    for (auto it = urls.begin(); it != urls.end(); ++it) {
        std::string const & url = it->second;
        if (not_canceled.count(url) || unique.count(url))
            continue;
        ret.insert(url.c_str());
    }
    return ret;
}
