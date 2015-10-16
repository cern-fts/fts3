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

#include "JobContext.h"

#include "cred/DelegCred.h"
#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_set>

#include "common/Logger.h"
#include "cred/CredUtility.h"


JobContext::JobContext(std::string const & dn, std::string const & vo, std::string const & delegationId, std::string const & spaceToken) : spaceToken(spaceToken)
{
    proxy = DelegCred::getProxyFile(dn, delegationId);
}

void JobContext::add(std::string const & surl, std::string const & jobId, int fileId)
{
    if(!surl.empty() && !jobId.empty() && fileId > 0)
        {
            jobs[jobId][surl].push_back(fileId);
            urlToIDs.insert({surl, {jobId, fileId}});
        }
}

bool JobContext::checkValidProxy(std::string& message) const
{
    return DelegCred::isValidProxy(proxy, message);
}

std::set<std::string> JobContext::getUrls() const
{
    std::set<std::string> ret;
    // make sure only unique SURLs will be brought online / deleted
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            auto const & urls = it_j->second;
            for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u)
                {
                    ret.insert(it_u->first);
                }
        }
    return ret;
}

void JobContext::removeUrl(const std::string& url)
{
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            it_j->second.erase(url);
        }

    urlToIDs.erase(url);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Removed " << url
                                     << " from the watch list"
                                     << fts3::common::commit;
}



std::string JobContext::getLogMsg() const
{
    std::stringstream ss;
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            ss << it_j->first << " (";
            for (auto it_u = it_j->second.begin(); it_u != it_j->second.end(); ++it_u)
                for (auto it_f = it_u->second.begin(); it_f != it_u->second.end(); ++it_f)
                    {
                        ss << *it_f << ", ";
                    }
            ss << "), ";
        }
    return ss.str();
}
