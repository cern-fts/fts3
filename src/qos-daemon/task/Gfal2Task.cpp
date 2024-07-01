/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Gfal2Task.h"
#include "../context/JobContext.h"

using namespace fts3::common;

std::string Gfal2Task::infosys;
bool Gfal2Task::http_log_content;


void Gfal2Task::setProxy(const JobContext &ctx)
{
    GError *error = NULL;

    //before any operation, check if the proxy is valid
    std::string message;
    bool isValid = ctx.checkValidProxy(message);
    if (!isValid) {
        ctx.updateState("FAILED", JobError(gfal2_ctx.operation, EACCES, message));
        std::stringstream ss;
        ss << gfal2_ctx.operation << " proxy certificate not valid: " << message;
        throw UserError(ss.str());
    }

    char* cert = const_cast<char*>(ctx.getProxy().c_str());

    int status = gfal2_set_opt_string(gfal2_ctx, "X509", "CERT", cert, &error);
    if (status < 0) {
        ctx.updateState("FAILED", JobError(gfal2_ctx.operation, error));
        std::stringstream ss;
        ss << gfal2_ctx.operation
            << " setting X509 CERT failed " << error->code << " "
            << error->message;
        g_clear_error(&error);
        throw UserError(ss.str());
    }

    status = gfal2_set_opt_string(gfal2_ctx, "X509", "KEY", cert, &error);
    if (status < 0) {
        ctx.updateState("FAILED", JobError(gfal2_ctx.operation, error));
        std::stringstream ss;
        ss << gfal2_ctx.operation
            << " setting X509 KEY failed " << error->code << " "
            << error->message;
        g_clear_error(&error);
        throw UserError(ss.str());
    }
}


void Gfal2Task::setSpaceToken(std::string const & spaceToken)
{
    // set up the gfal2 context
    GError *error = NULL;

    if (!spaceToken.empty()) {
        gfal2_set_opt_string(gfal2_ctx,
            "SRM PLUGIN", "SPACETOKENDESC", (char *) spaceToken.c_str(),
            &error);
        if (error) {
            std::stringstream ss;
            ss << gfal2_ctx.operation << " could not set the destination space token "
                << error->code << " " << error->message;
            g_clear_error(&error);
            throw UserError(ss.str());
        }
    }
}

void Gfal2Task::setToken(std::string const & token)
{
	// set up the gfal2 context
	GError *error = NULL;
	if (!token.empty()) {
            int status = gfal2_set_opt_string(gfal2_ctx, "BEARER", "TOKEN", (char *) token.c_str(), &error);
            if (status) {
		std::stringstream ss;
		ss << gfal2_ctx.operation << " could not set the token "
		    		<< error->code << " " << error->message;
		g_clear_error(&error);
		throw UserError(ss.str());
            }
	}

}
