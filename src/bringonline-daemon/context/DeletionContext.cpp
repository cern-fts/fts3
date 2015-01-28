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

    add(surl, jobId, fileId);
}
