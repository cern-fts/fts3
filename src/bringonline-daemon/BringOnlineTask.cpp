/*
 * BringOnlineTask.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#include "BringOnlineTask.h"
#include "PollTask.h"

#include "WaitingRoom.h"

#include "common/logger.h"
#include "common/error.h"

BringOnlineTask::BringOnlineTask(std::pair<key_type, StagingContext> const & ctx) : StagingTask(ctx.second)
{
    // set up the gfal2 context
    GError *error = NULL;

    const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};

    gfal2_set_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
    if (error)
        {
            std::stringstream ss;
            ss << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    gfal2_set_opt_boolean(gfal2_ctx, "GRIDFTP PLUGIN", "SESSION_REUSE", true, &error);
    if (error)
        {
            std::stringstream ss;
            ss << "BRINGONLINE Could not set the session reuse " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    if (!std::get<space_token>(ctx.first).empty())
        {
            gfal2_set_opt_string(gfal2_ctx, "SRM PLUGIN", "SPACETOKENDESC", (char *) std::get<space_token>(ctx.first).c_str(), &error);
            if (error)
                {
                    std::stringstream ss;
                    ss << "BRINGONLINE Could not set the space token " << error->code << " " << error->message;
                    throw Err_Custom(ss.str());
                }
        }

    // set the proxy certificate
    setProxy();
}

void BringOnlineTask::setProxy()
{
    GError *error = NULL;

    //before any operation, check if the proxy is valid
    std::string message;
    bool isValid = StagingContext::checkValidProxy(ctx.proxy, message);
    if(!isValid)
        {
            state_update(ctx.jobs, "FAILED", message, false);
            std::stringstream ss;
            ss << "BRINGONLINE proxy certificate not valid: " << message;
            throw Err_Custom(ss.str());
        }

    char* cert = const_cast<char*>(ctx.proxy.c_str());

    int status = gfal2_set_opt_string(gfal2_ctx, "X509", "CERT", cert, &error);
    if (status < 0)
        {
            state_update(ctx.jobs, "FAILED", error->message, false);
            std::stringstream ss;
            ss << "BRINGONLINE setting X509 CERT failed " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    status = gfal2_set_opt_string(gfal2_ctx, "X509", "KEY", cert, &error);
    if (status < 0)
        {
            state_update(ctx.jobs, "FAILED", error->message, false);
            std::stringstream ss;
            ss << "BRINGONLINE setting X509 KEY failed " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }
}

void BringOnlineTask::run(boost::any const &)
{
    GError *error = NULL;
    char token[512] = {0};

    std::vector<char const *> urls = ctx.getUrls();
    int status = gfal2_bring_online_list(
                     gfal2_ctx,
                     urls.size(),
                     &*urls.begin(),
                     ctx.pinlifetime,
                     ctx.bringonlineTimeout,
                     token,
                     sizeof(token),
                     1,
                     &error
                 );

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE FAILED "
                                           << ctx.getLogMsg() << " "
                                           << error->code << " " << error->message  << commit;

            bool retry = retryTransfer(error->code, "SOURCE", std::string(error->message));
            state_update(ctx.jobs, "FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else if (status == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token  << " "
                                            << ctx.getLogMsg() << commit;

            state_update(ctx.jobs, token);
            WaitingRoom<PollTask>::instance().add(new PollTask(*this, token));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED, got token " << token   << " "
                                            << ctx.getLogMsg() <<  commit;
            // No need to poll in this case!
            state_update(ctx.jobs, "FINISHED", "", false);
        }
}

