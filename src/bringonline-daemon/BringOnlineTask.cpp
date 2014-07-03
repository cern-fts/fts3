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

BringOnlineTask::BringOnlineTask(context_type const & ctx, std::string const & proxy) : StagingTask(ctx), proxy(proxy)
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

    if (!boost::get<src_space_token>(ctx).empty())
        {
            gfal2_set_opt_string(gfal2_ctx, "SRM PLUGIN", "SPACETOKENDESC", (char *) boost::get<src_space_token>(ctx).c_str(), &error);
            if (error)
                {
                    std::stringstream ss;
                    ss << "BRINGONLINE Could not set the space token " << error->code << " " << error->message;
                    throw Err_Custom(ss.str());
                }
        }

    if (infosys == "false")
        {
            gfal2_set_opt_boolean(gfal2_ctx, "BDII", "ENABLED", false, NULL);
        }
    else
        {
            gfal2_set_opt_string(gfal2_ctx, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
        }

    // set the proxy certificate
    setProxy();
}

void BringOnlineTask::setProxy()
{
    GError *error = NULL;

    //before any operation, check if the proxy is valid
    std::string message;
    bool isValid = checkValidProxy(proxy, message);
    if(!isValid)
        {
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FAILED", message, false);
            std::stringstream ss;
            ss << "BRINGONLINE proxy certificate not valid: " << message;
            throw Err_Custom(ss.str());
        }

    char* cert = const_cast<char*>(proxy.c_str());

    int status = gfal2_set_opt_string(gfal2_ctx, "X509", "CERT", cert, &error);
    if (status < 0)
        {
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FAILED", error->message, false);
            std::stringstream ss;
            ss << "BRINGONLINE setting X509 CERT failed " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    status = gfal2_set_opt_string(gfal2_ctx, "X509", "KEY", cert, &error);
    if (status < 0)
        {
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FAILED", error->message, false);
            std::stringstream ss;
            ss << "BRINGONLINE setting X509 KEY failed " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }
}

void BringOnlineTask::run(boost::any const &)
{
    long int pinlifetime = 28800;
    long int bringonlineTimeout = 28800;

    if(boost::get<copy_pin_lifetime>(ctx) > pinlifetime)
        {
            pinlifetime = boost::get<copy_pin_lifetime>(ctx);
        }

    if(boost::get<bring_online_timeout>(ctx) > bringonlineTimeout)
        {
            bringonlineTimeout = boost::get<bring_online_timeout>(ctx);
        }

    GError *error = NULL;
    char token[512] = {0};
    int status = gfal2_bring_online(gfal2_ctx, boost::get<url>(ctx).c_str(), pinlifetime, bringonlineTimeout, token, sizeof(token), 1, &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE FAILED "
                                           << boost::get<job_id>(ctx) << " "
                                           << boost::get<file_id>(ctx) <<  " "
                                           << error->code << " " << error->message  << commit;

            bool retry = retryTransfer(error->code, "SOURCE", std::string(error->message));
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else if (status == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token  << " "
                                            << boost::get<job_id>(ctx) << " "
                                            << boost::get<file_id>(ctx) <<  commit;
            WaitingRoom<PollTask>::instance().add(new PollTask(*this, token));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED, got token " << token   << " "
                                            << boost::get<job_id>(ctx) << " "
                                            << boost::get<file_id>(ctx) <<  commit;
            // No need to poll in this case!
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FINISHED", "", false);
        }
}

