/*
 * PollTask.cpp
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#include "PollTask.h"

#include "ThreadContext.h"
#include "WaitingRoom.h"

#include "common/logger.h"

#include <gfal_api.h>

using namespace FTS3_COMMON_NAMESPACE;

void PollTask::run(boost::any const & thread_ctx)
{
	// get the gfal2 gfal2_ctx
	ThreadContext const & gfal2_ctx = boost::any_cast<ThreadContext const &>(thread_ctx);

	setProxy(gfal2_ctx.get());

	GError *error = NULL;
	int status = gfal2_bring_online_poll(gfal2_ctx.get(), ctx.url.c_str(), ctx.token.c_str(), &error);

	if (status < 0)
		{
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling failed, token " << ctx.token << ", "  << error->code << " " << error->message << commit;
			if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && ctx.retries < 3 )
				{
					FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
					ctx.retries +=1;
					ctx.started = false;
					WaitingRoom::get().add(new PollTask(ctx));
				}
			else
				{
					db.bringOnlineReportStatus("FAILED", std::string(error->message), ctx);
					ctx.started = true;
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
			WaitingRoom::get().add(new PollTask(ctx));
		}
	else
		{
			FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE finished token " << ctx.token << commit;
			ctx.started = true;
			db.bringOnlineReportStatus("FINISHED", "", ctx);
		}
}

