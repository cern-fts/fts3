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

#include <algorithm>
#include <memory>
#include <set>
#include <sstream>

#include "common/Logger.h"
#include "cred/CredUtility.h"
#include "cred/DelegCred.h"

#include "JobContext.h"

JobError::JobError(): error(NULL)
{
}


JobError::JobError(const std::string &operation, const GError *error):
    operation(operation), error(g_error_copy(error))
{
}


JobError::JobError(const std::string &operation, int code, const std::string &error):
    operation(operation), error(g_error_new_literal(g_quark_from_static_string(""), code, error.c_str()))
{
}


JobError::JobError(const JobError &e): operation(e.operation), error(g_error_copy(e.error))
{

}


JobError::~JobError()
{
    if (error != NULL) {
        g_error_free(error);
    }
}


std::string JobError::String() const
{
    if (error == NULL) {
       return std::string();
    }

    std::stringstream s;
    s << operation << " [" << error->code << "] " << error->message;
    return s.str();
}


bool JobError::IsRecoverable() const
{
    if (error == NULL) {
        return false;
    }
    switch (error->code)
    {
        case ETIMEDOUT:
        case ECONNABORTED:
        case ECONNREFUSED:
        case ECONNRESET:
        case EAGAIN:
        case EBUSY:
            return true;
        default:
            return false;
    }
}


JobContext::JobContext(const std::string &dn, const std::string &vo,
    const std::string &delegationId, const std::string &spaceToken) :
    spaceToken(spaceToken)
{
    (void) vo;
    proxy = DelegCred::getProxyFile(dn, delegationId);
}


void JobContext::add(std::string const & surl, std::string const & jobId, uint64_t fileId)
{
    if (!surl.empty() && !jobId.empty() && fileId > 0) {
        jobs[jobId][surl].push_back(fileId);
        urlToIDs.insert(
        { surl,
        { jobId, fileId } });
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
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j) {
        auto const & urls = it_j->second;
        for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u) {
            ret.insert(it_u->first);
        }
    }
    return ret;
}


void JobContext::removeUrl(const std::string& url)
{
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j) {
        it_j->second.erase(url);
    }

    urlToIDs.erase(url);

    FTS3_COMMON_LOGGER_NEWLOG(DEBUG)<< "Removed " << url
        << " from the watch list"
        << fts3::common::commit;
}


std::string JobContext::getLogMsg() const
{
    std::stringstream ss;

    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j) {
        if (it_j != jobs.begin()) {
            ss << ",";
        }

        ss << it_j->first << " (";
        for (auto it_u = it_j->second.begin(); it_u != it_j->second.end(); ++it_u)
            for (auto it_f = it_u->second.begin(); it_f != it_u->second.end(); ++it_f) {
                if (it_f != it_u->second.begin()) {
                    ss << ", ";
                }

                ss << *it_f;
            }
        ss << ")";
    }

    return ss.str();
}
