/*
 * PollTask.cpp
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#include "PollTask.h"

#include "BringOnlineTask.h"
#include "WaitingRoom.h"

#include "common/logger.h"

#include <gfal_api.h>

void PollTask::run(boost::any const &)
{
    GError *error = NULL;
    int status = gfal2_bring_online_poll(gfal2_ctx, ctx.url.c_str(), ctx.token.c_str(), &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling failed, token " << ctx.token << ", "  << error->code << " " << error->message << commit;
            if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && ctx.retries < 3 )
                {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
                    ctx.retries +=1;
                    ctx.started = false;
                    WaitingRoom::instance().add(new BringOnlineTask(*this));
                }
            else
                {
                    db.bringOnlineReportStatus("FAILED", std::string(error->message), ctx);
                }
            g_clear_error(&error);
        }
    else if(status == 0)
        {
            time_t interval = getPollInterval(++ctx.nPolls), now = time(NULL);
            ctx.nextPoll = now + interval;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling token " << ctx.token << commit;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
            ctx.started = true;
            WaitingRoom::instance().add(new PollTask(*this));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE finished token " << ctx.token << commit;
            db.bringOnlineReportStatus("FINISHED", "", ctx);
        }
}

