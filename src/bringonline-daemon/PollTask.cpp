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

boost::shared_mutex PollTask::mx;

std::set<std::string> PollTask::active_tokens;

void PollTask::run(boost::any const &)
{
	if (!active())
	{
		abort();
		return;
	}

	std::vector<char const *> urls = ctx.getUrls();

    GError *error = NULL;
    int status = gfal2_bring_online_poll_list(gfal2_ctx, urls.size(), &*urls.begin(), token.c_str(), &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling FAILED "
                                           << ctx.getLogMsg()
                                           << token << " "
                                           << error->code << " "
                                           << error->message << commit;

            bool retry = retryTransfer(error->code, "SOURCE", error->message);
            state_update(ctx, "FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else if(status == 0)
        {
            time_t interval = getPollInterval(++nPolls), now = time(NULL);
            wait_until = now + interval;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling "
                                            << ctx.getLogMsg()
                                            << token << commit;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
            WaitingRoom<PollTask>::instance().add(new PollTask(*this));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED  "
                                            << ctx.getLogMsg()
                                            << token << commit;

            state_update(ctx, "FINISHED", "", false);
        }
}

void PollTask::abort()
{
	std::vector<char const *> urls = ctx.getUrls();

	GError * err;
	int status = gfal2_abort_files(gfal2_ctx, urls.size(), &*urls.begin(), token.c_str(), &err);

	if (status < 0)
	{
		FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE abort FAILED "
									   << ctx.getLogMsg()
									   << err->code << " " << err->message  << commit;
		g_clear_error(&err);
	}
}

