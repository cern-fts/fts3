/*
 * BringOnlineTask.cpp
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#include "BringOnlineTask.h"

#include "ThreadContext.h"
#include "WaitingRoom.h"

#include "common/logger.h"

using namespace FTS3_COMMON_NAMESPACE;

void BringOnlineTask::run(boost::any const & thread_ctx)
{
	// get the gfal2 context
	ThreadContext const & gfal2_ctx = boost::any_cast<ThreadContext const &>(thread_ctx);

	setProxy(gfal2_ctx.get());

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
	int status = gfal2_bring_online(gfal2_ctx.get(), ctx.url.c_str(), pinlifetime, bringonlineTimeout, token, sizeof(token), 1, &error);

	if (status < 0)
		{
			FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE failed " << error->code << " " << error->message << commit;
			if(true == retryTransfer(error->code, "SOURCE", std::string(error->message)) && ctx.retries < 3 )
				{
					FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE will be retried" << commit;
					ctx.retries +=1;
					WaitingRoom::get().add(new BringOnlineTask(ctx));
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
			WaitingRoom::get().add(new PollTask(ctx));
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

