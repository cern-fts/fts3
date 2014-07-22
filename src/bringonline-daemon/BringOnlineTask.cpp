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

