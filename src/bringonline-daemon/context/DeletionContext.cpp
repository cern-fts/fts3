/*
 * DeletionContext.cpp
 *
 *  Created on: 17 Jul 2014
 *      Author: roiser
 */

#include "DeletionContext.h"

#include "cred/cred-utility.h"

#include <sstream>


void DeletionContext::add(context_type const & ctx)
{
    std::string const & surl = boost::get<source_url>(ctx);
    std::string const & jobId = boost::get<job_id>(ctx);
    int fileId = boost::get<file_id>(ctx);

    if (surl.compare(0, 6, "srm://") == 0)
        {
            srm_jobs[jobId][surl].push_back(fileId);
        }
    else
        add(surl, jobId, fileId);
}

std::vector<char const *> DeletionContext::getSrmUrls() const
{
    std::vector<char const *> ret;
    for (auto it_j = srm_jobs.begin(); it_j != srm_jobs.end(); ++it_j)
        {
            for (auto it_u = it_j->second.begin(); it_u != it_j->second.end(); ++it_u)
                {
                    ret.push_back(it_u->first.c_str());
                }
        }

    return ret;
}
