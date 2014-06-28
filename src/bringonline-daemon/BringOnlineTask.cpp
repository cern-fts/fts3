/*
 * BringOnlineTask.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#include "BringOnlineTask.h"

#include "WaitingRoom.h"

#include "common/logger.h"
#include "common/error.h"

BringOnlineTask::BringOnlineTask(message_bringonline ctx) : StagingTask(ctx)
{
    // Set up handle
    GError *error = NULL;
    gfal2_ctx = gfal2_context_new(&error);
    if (!gfal2_ctx)
        {
            std::stringstream ss;
            ss << "BRINGONLINE bad initialization " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
        }

    const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};

    gfal2_set_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
    if (error)
        {
            std::stringstream ss;
            ss << "BRINGONLINE Could not set the protocol list " << error->code << " " << error->message;
            throw Err_Custom(ss.str());
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

void BringOnlineTask::run(boost::any const &)
{
    long int pinlifetime = 28800;
    long int bringonlineTimeout = 28800;

    if(ctx.pinlifetime > pinlifetime)
        {
            pinlifetime = ctx.pinlifetime;
        }

    if(ctx.bringonlineTimeout > bringonlineTimeout)
        {
            bringonlineTimeout = ctx.bringonlineTimeout;
        }

    GError *error = NULL;
    char token[512] = {0};
    int status = gfal2_bring_online(gfal2_ctx, ctx.url.c_str(), pinlifetime, bringonlineTimeout, token, sizeof(token), 1, &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE failed " << error->code << " " << error->message << commit;
            if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && ctx.retries < 3)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
                    ctx.retries +=1;
                    WaitingRoom::instance().add(new BringOnlineTask(*this));
                }
            else
                {
                    db.bringOnlineReportStatus("FAILED", std::string(error->message), ctx);
                    ctx.started = true;
                }
            g_clear_error(&error);
        }
    else if (status == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token << commit;
            ctx.token = std::string(token);
            db.addToken(ctx.job_id, ctx.file_id, ctx.token);
            ctx.started = true;
            ctx.retries = 0;
            WaitingRoom::instance().add(new PollTask(*this));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE succeeded, got token " << token << commit;
            db.addToken(ctx.job_id, ctx.file_id, std::string(token));
            ctx.started = true;
            ctx.retries = 0;
            // No need to poll in this case!
            db::DBSingleton::instance().getDBObjectInstance()->bringOnlineReportStatus("FINISHED", "", ctx);
        }
}

