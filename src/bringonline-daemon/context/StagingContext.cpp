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
    for (auto it = jobs.begin(); it != jobs.end();)
        {
            auto & files = it->second;
            auto store = files.begin();
            for (auto it_f = files.begin(); it_f != files.end(); ++it_f)
            {
                if (!urls.count(it_f->second))
                {
                    *store = *it_f;
                    ++store;
                }
            }
            files.resize(store - files.begin());
            if (files.empty()) jobs.erase(it++);
            else ++it;
        }
}
