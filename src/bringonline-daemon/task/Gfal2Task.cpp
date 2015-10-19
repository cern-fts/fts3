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


void Gfal2Task::setProxy(const JobContext &ctx)
{
    GError *error = NULL;

    //before any operation, check if the proxy is valid
    std::string message;
    bool isValid = ctx.checkValidProxy(message);
    if (!isValid) {
        ctx.updateState("FAILED", message, false);
        std::stringstream ss;
        ss << gfal2_ctx.operation << " proxy certificate not valid: " << message;
        throw UserError(ss.str());
    }

    char* cert = const_cast<char*>(ctx.getProxy().c_str());

    int status = gfal2_set_opt_string(gfal2_ctx, "X509", "CERT", cert, &error);
    if (status < 0) {
        ctx.updateState("FAILED", error->message, false);
        std::stringstream ss;
        ss << gfal2_ctx.operation
            << " setting X509 CERT failed " << error->code << " "
            << error->message;
        g_clear_error(&error);
        throw UserError(ss.str());
    }

    status = gfal2_set_opt_string(gfal2_ctx, "X509", "KEY", cert, &error);
    if (status < 0) {
        ctx.updateState("FAILED", error->message, false);
        std::stringstream ss;
        ss << gfal2_ctx.operation
            << " setting X509 KEY failed " << error->code << " "
            << error->message;
        g_clear_error(&error);
        throw UserError(ss.str());
    }
}


bool Gfal2Task::doRetry(int errorNo, const std::string& category, const std::string& message)
{
    bool retry = true;

    // ETIMEDOUT shortcuts the following
    if (errorNo == ETIMEDOUT)
        return true;

    //search for error patterns not reported by SRM or GSIFTP
    std::size_t found = message.find("performance marker");
    if (found!=std::string::npos)
        return true;
    found = message.find("Connection timed out");
    if (found!=std::string::npos)
        return true;
    found = message.find("end-of-file was reached");
    if (found!=std::string::npos)
        return true;

    if (category == "SOURCE") {
        switch (errorNo)
        {
        case ENOENT: // No such file or directory
        case EPERM: // Operation not permitted
        case EACCES: // Permission denied
        case EISDIR: // Is a directory
        case ENAMETOOLONG: // File name too long
        case E2BIG: // Argument list too long
        case ENOTDIR: // Part of the path is not a directory
        case EPROTONOSUPPORT: // Protocol not supported by gfal2 (plugin missing?)
        case EINVAL: // Invalid argument. i.e: invalid request token
            retry = false;
            break;
        default:
            retry = true;
            break;
        }
    }

    found = message.find("proxy expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("File exists and overwrite");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("No such file or directory");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_INVALID_PATH");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The certificate has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("The available CRL has expired");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM Authentication failed");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_DUPLICATION_ERROR");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_AUTHENTICATION_FAILURE");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("SRM_NO_FREE_SPACE");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("digest too big for rsa key");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("Can not determine address of local host");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("Permission denied");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("System error in write into HDFS");
    if (found!=std::string::npos)
        retry = false;
    found = message.find("globus_ftp_client: the server responded with an error 530"); // Authentication error
    if (found!=std::string::npos)
        retry = false;

    return retry;
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
