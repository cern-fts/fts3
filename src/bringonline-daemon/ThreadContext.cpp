/*
 * ThreadContext.cpp
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#include "ThreadContext.h"

#include "common/error.h"
#include "common/logger.h"

using namespace FTS3_COMMON_NAMESPACE;

ThreadContext::ContextPrototype ThreadContext::prototype;

ThreadContext::ThreadContext(ThreadContext const & prototype) : gfal2_handle(0)
{
	infosys = prototype.infosys;

    // Set up handle
    GError *error = NULL;
    gfal2_context_t handle = gfal2_context_new(&error);
    if (!handle)
        {
    		std::stringstream ss;
            ss << "BRINGONLINE bad initialization " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};

    gfal2_set_opt_string_list(handle, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
    if (error)
        {
    		std::stringstream ss;
            ss << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    if (infosys.compare("false") == 0)
        {
            gfal2_set_opt_boolean(handle, "BDII", "ENABLED", false, NULL);
        }
    else
        {
            gfal2_set_opt_string(handle, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
        }
}

ThreadContext::~ThreadContext()
{
	if(gfal2_handle) gfal2_context_free(gfal2_handle);
}

