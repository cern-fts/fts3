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

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE issuing bring-online for: " << urls.size() << " files, "
                                    << "with copy-pin-lifetime: " << ctx.getPinlifetime() <<
                                    " and bring-online-timeout: " << ctx.getBringonlineTimeout() << commit;

    int status = gfal2_bring_online_list(
                     gfal2_ctx,
                     urls.size(),
                     &*urls.begin(),
                     ctx.getPinlifetime(),
                     ctx.getBringonlineTimeout(),
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

            bool retry = doRetry(error->code, "SOURCE", std::string(error->message));
            ctx.state_update("FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else if (status == 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token  << " "
                                            << ctx.getLogMsg() << commit;

            ctx.state_update(token);
            WaitingRoom<PollTask>::instance().add(new PollTask(std::move(*this), token));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED, got token " << token   << " "
                                            << ctx.getLogMsg() <<  commit;
            // No need to poll in this case!
            ctx.state_update("FINISHED", "", false);
        }
}

