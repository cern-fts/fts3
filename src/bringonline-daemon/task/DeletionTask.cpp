/*
 * DeletionTask.cpp
 *
 *  Created on: 17 July 2014
 *      Author: roiser
 */

#include "DeletionTask.h"
#include "WaitingRoom.h"

#include "common/logger.h"
#include "common/error.h"

#include <unordered_map>

DeletionTask::DeletionTask(DeletionContext const & ctx) : Gfal2Task("DELETION"), ctx(ctx)
{
    // set the proxy certificate
    setProxy(ctx);
}

void DeletionTask::run(boost::any const &)
{
    run_impl();
}

void DeletionTask::run_impl()
{
    std::vector<char const *> urls = ctx.getUrls();
    // make sure there is work to do
    if (urls.empty()) return;
    std::vector<GError*> error (urls.size(), NULL);
    int status = gfal2_unlink_list(gfal2_ctx, (int)urls.size(), urls.data(), error.data());

    if (status < 0)
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    std::vector< std::pair<std::string, int> > const ids = ctx.getIDs(urls[i]);

                    if (error[i])
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION FAILED " << urls[i] << ": "
                                                           << error[i]->code << " " << error[i]->message
                                                           << commit;

                            bool retry = doRetry(error[i]->code, "SOURCE", std::string(error[i]->message));
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", error[i]->message, retry);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "DELETION FAILED for " << urls[i]
                                                            << ": returned -1 but error was not set ";
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", "Error not set by gfal2", false);
                        }
                    g_clear_error(&error[i]);
                }
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION FINISHED "
                                            << ctx.getLogMsg() <<  commit;
            ctx.state_update("FINISHED", "", false);
        }
}
