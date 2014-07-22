/*
 * StagingContext.cpp
 *
 *  Created on: 10 Jul 2014
 *      Author: simonm
 */

#include "StagingContext.h"

#include "cred/cred-utility.h"

#include <sstream>


void StagingContext::add(context_type const & ctx)
{
    if(boost::get<copy_pin_lifetime>(ctx) > pinlifetime)
        {
            pinlifetime = boost::get<copy_pin_lifetime>(ctx);
        }

    if(boost::get<bring_online_timeout>(ctx) > bringonlineTimeout)
        {
            bringonlineTimeout = boost::get<bring_online_timeout>(ctx);
        }

    surls.insert(boost::get<surl>(ctx));

    if (delegationId.empty()) delegationId = boost::get<dlg_id>(ctx);

    jobs[boost::get<job_id>(ctx)].push_back(boost::get<file_id>(ctx));

    if (proxy.empty())
        {
            proxy = generateProxy(boost::get<dn>(ctx), delegationId);

            //get the proxy
            std::string message;
            if(!checkValidProxy(proxy, message))
                {
                    proxy = get_proxy_cert(
                                boost::get<dn>(ctx), // user_dn
                                boost::get<dlg_id>(ctx), // user_cred
                                boost::get<vo>(ctx), // vo_name
                                "",
                                "", // assoc_service
                                "", // assoc_service_type
                                false,
                                ""
                            );
                }
        }

    if (srcSpaceToken.empty())
        {
            srcSpaceToken = boost::get<src_space_token>(ctx);
        }
}

std::vector<char const *> StagingContext::getUrls() const
{
    std::vector<char const *> ret;
    ret.reserve(surls.size());

    std::set<std::string>::const_iterator it;
    for (it = surls.begin(); it != surls.end(); ++it)
        {
            ret.push_back(it->c_str());
        }

    return ret;
}

std::string StagingContext::getLogMsg() const
{
    std::stringstream ss;

    std::map< std::string, std::vector<int> >::const_iterator it_j;
    std::vector<int>::const_iterator it_f;

    for (it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        {
            // get the first file ID
            it_f = it_j->second.begin();
            if (it_f == it_j->second.end()) continue;
            // create the message
            ss << it_j->first << " (" << *it_f;
            ++it_f;
            // add subsequent file IDs
            for (; it_f != it_j->second.end(); ++it_f)
                {
                    ss << ", " << *it_f;
                }
            // add closing parenthesis
            ss << ") ";
        }

    return ss.str();
}


