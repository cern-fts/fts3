/*
 * JobContext.cpp
 *
 *  Created on: 15 Sep 2014
 *      Author: simonm
 */

#include "JobContext.h"

#include "cred/DelegCred.h"
#include "cred/cred-utility.h"
#include "common/logger.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <unordered_set>

JobContext::JobContext(std::string const & dn, std::string const & vo, std::string const & delegationId, std::string const & spaceToken) : spaceToken(spaceToken)
{
    proxy = generateProxy(dn, delegationId);

    //get the proxy
    std::string message;
    if(!checkValidProxy(message))
        proxy = get_proxy_cert(dn, delegationId, vo, "", "", "", false, "");
}

void JobContext::add(std::string const & surl, std::string const & jobId, int fileId)
{
    if(!surl.empty() && !jobId.empty() && fileId > 0)
        {
            jobs[jobId][surl].push_back(fileId);
            urlToIDs.insert({surl, {jobId, fileId}});
        }
}

std::string JobContext::generateProxy(std::string const & dn, std::string const & delegationId)
{
    std::unique_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->getFileName(dn, delegationId);
}

bool JobContext::checkValidProxy(std::string& message) const
{
    std::unique_ptr<DelegCred> delegCredPtr(new DelegCred);
    return delegCredPtr->isValidProxy(proxy, message);
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
