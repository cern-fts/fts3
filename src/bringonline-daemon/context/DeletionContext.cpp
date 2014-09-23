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
        srm_jobs[jobId].push_back(std::make_pair(fileId, surl));
    else
        add(surl, jobId, fileId);
}

std::vector<char const *> DeletionContext::getSrmUrls() const
{
    std::vector<char const *> ret;

    std::map< std::string, std::vector<std::pair<int, std::string> > >::const_iterator it_j;
    std::vector<std::pair<int, std::string> >::const_iterator it_f;

    for (it_j = srm_jobs.begin(); it_j != srm_jobs.end(); ++it_j)
        {
            for (it_f = it_j->second.begin(); it_f != it_j->second.end(); ++it_f)
                {
                    ret.push_back(it_f->second.c_str());
                }
        }

    return ret;
}
