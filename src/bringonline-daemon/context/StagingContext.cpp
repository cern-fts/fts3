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

    std::string url = boost::get<surl>(ctx);
    std::string jobId = boost::get<job_id>(ctx);
    int fileId =  boost::get<file_id>(ctx);
    add(url, jobId, fileId);
}

bool StagingContext::is_timeouted()
{
    return difftime(time(0), start_time) > bringonlineTimeout;
}

void StagingContext::remove(std::set<std::string> const & urls)
{
    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
        for (auto it_u = urls.begin(); it_u != urls.end(); ++it_u)
            it_j->second.erase(*it_u);
}
