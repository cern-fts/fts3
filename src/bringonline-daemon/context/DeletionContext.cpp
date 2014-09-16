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
    add(boost::get<source_url>(ctx), boost::get<job_id>(ctx), boost::get<file_id>(ctx));
}
