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
    char token[512] = {0};
    std::vector<char const *> urls = ctx.getUrls();
    std::vector<GError*> errors(urls.size(), NULL);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE issuing bring-online for: " << urls.size() << " files, "
                                    << "with copy-pin-lifetime: " << ctx.getPinlifetime() <<
                                    " and bring-online-timeout: " << ctx.getBringonlineTimeout() << commit;

    int status = gfal2_bring_online_list(
                     gfal2_ctx,
                     static_cast<int>(urls.size()),
                     &*urls.begin(),
                     ctx.getPinlifetime(),
                     ctx.getBringonlineTimeout(),
                     token,
                     sizeof(token),
                     1,
                     &errors.front()
                 );

    if (status < 0)
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    std::pair<std::string, int> ids = ctx.getIDs(urls[i]);

                    if (errors[i])
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE FAILED for " << urls[i] << ": "
                                                           << errors[i]->code << " " << errors[i]->message
                                                           << commit;

                            bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                            ctx.state_update(ids.first, ids.second, "FAILED", errors[i]->message, retry);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "BRINGONLINE FAILED for " << urls[i]
                                           << ": returned -1 but error was not set ";
                            ctx.state_update(ids.first, ids.second, "FAILED", "Error not set by gfal2", false);
                        }
                    g_clear_error(&errors[i]);
                }
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
            // No need to poll
            for (size_t i = 0; i < urls.size(); ++i)
            {
                std::pair<std::string, int> ids = ctx.getIDs(urls[i]);

                if (errors[i]) {
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE FAILED for " << urls[i] << ": "
                                                   << errors[i]->code << " " << errors[i]->message
                                                   << commit;

                    bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                    ctx.state_update(ids.first, ids.second, "FAILED", errors[i]->message, retry);
                    g_clear_error(&errors[i]);
                }
                else {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED for "
                                                    << urls[i] << " , got token " << token
                                                    << commit;
                    ctx.state_update(ids.first, ids.second, "FINISHED", "", false);
                }
            }
        }
}

