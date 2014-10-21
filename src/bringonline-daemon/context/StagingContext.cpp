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


