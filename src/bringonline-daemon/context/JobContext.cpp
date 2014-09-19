/*
 * JobContext.cpp
 *
 *  Created on: 15 Sep 2014
 *      Author: simonm
 */

#include "JobContext.h"

#include "cred/DelegCred.h"
#include "cred/cred-utility.h"

#include <memory>
#include <sstream>

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
    jobs[jobId].push_back(std::make_pair(fileId, surl));
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


std::vector<char const *> JobContext::getUrls() const
{
    std::vector<char const *> ret;

    std::map< std::string, std::vector<std::pair<int, std::string> > >::const_iterator it_j;
    std::vector<std::pair<int, std::string> >::const_iterator it_f;

    for (it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            for (it_f = it_j->second.begin(); it_f != it_j->second.end(); ++it_f)
                {
                    ret.push_back(it_f->second.c_str());
                }
        }

    return ret;
}

std::string JobContext::getLogMsg() const
{
    std::stringstream ss;

    std::map< std::string, std::vector<std::pair<int, std::string> > >::const_iterator it_j;
    std::vector< std::pair<int, std::string> >::const_iterator it_f;

    for (it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            // get the first file ID
            it_f = it_j->second.begin();
            if (it_f == it_j->second.end()) continue;
            // create the message
            ss << it_j->first << " (" << it_f->first;
            ++it_f;
            // add subsequent file IDs
            for (; it_f != it_j->second.end(); ++it_f)
                {
                    ss << ", " << it_f->first;
                }
            // add closing parenthesis
            ss << ") ";
        }

    return ss.str();
}
