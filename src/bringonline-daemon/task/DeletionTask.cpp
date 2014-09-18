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

DeletionTask::DeletionTask(DeletionContext const & ctx) : Gfal2Task("DELETION"), ctx(ctx)
{
    // set the proxy certificate
    setProxy(ctx);
}

void DeletionTask::run(boost::any const &)
{
    run_srm();
}

void DeletionTask::run()
{
    GError *error = NULL;

    std::vector<char const *> urls = ctx.getUrls();
    std::vector<char const *>::const_iterator it;

    int status = 0;
    for (it = urls.begin(); it != urls.end(); ++it)
        {
            status = gfal2_unlink(gfal2_ctx, *it, &error);
        }
}

void DeletionTask::run_srm()
{
    GError *error = NULL;

    std::vector<char const *> urls = ctx.getUrls();
    int status = gfal2_unlink_list(gfal2_ctx, urls.size(), &*urls.begin(), &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION FAILED "
                                           << ctx.getLogMsg() << " "
                                           << error->code << " "
                                           << error->message  << commit;

            bool retry = doRetry(error->code, "SOURCE", std::string(error->message));
            ctx.state_update("FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION FINISHED "
                                            << ctx.getLogMsg() <<  commit;
            // No need to poll in this case!
            ctx.state_update("FINISHED", "", false);
        }
}
