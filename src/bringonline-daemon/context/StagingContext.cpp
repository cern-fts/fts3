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
    submit_time[std::make_pair(jobId, fileId)] = boost::get<timestamp>(ctx);
    add(url, jobId, fileId);
}

std::map< std::string, std::vector<std::pair<int, std::string> > > StagingContext::get_timeouted()
{
    std::map< std::string, std::vector<std::pair<int, std::string> > > timeouted;
    time_t now = time(0);

    for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
    {
        for (auto it = it_j->second.begin(); it != it_j->second.end(); ++it)
        {
            if (difftime(now, submit_time[std::make_pair(it_j->first, it->first)]) > bringonlineTimeout)
                timeouted[it_j->first].push_back(*it);
        }
    }
    // remove from jobs
    for (auto it = timeouted.begin(); it != timeouted.end(); ++it)
    {
        jobs[it->first].erase(it->second.begin(), it->second.end());
        if (jobs[it->first].empty()) jobs.erase(it->first);
    }
    return timeouted;
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
