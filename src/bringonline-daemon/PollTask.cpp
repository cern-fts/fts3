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
    int status = gfal2_bring_online_poll(gfal2_ctx, boost::get<url>(ctx).c_str(), token.c_str(), &error);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling FAILED "
                                           << boost::get<job_id>(ctx) << " "
                                           << boost::get<file_id>(ctx) <<  "  "
                                           << token << " "
                                           << error->code << " "
                                           << error->message << commit;

            bool retry = retryTransfer(error->code, "SOURCE", error->message);
            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FAILED", error->message, retry);
            g_clear_error(&error);
        }
    else if(status == 0)
        {
            time_t interval = getPollInterval(++nPolls), now = time(NULL);
            wait_until = now + interval;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling "
                                            << boost::get<job_id>(ctx) << " "
                                            << boost::get<file_id>(ctx) <<  "  "
                                            << token << commit;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
            WaitingRoom<PollTask>::instance().add(new PollTask(*this));
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED  "
                                            << boost::get<job_id>(ctx) << " "
                                            << boost::get<file_id>(ctx) <<  "  "
                                            << token << commit;

            state_update(boost::get<job_id>(ctx), boost::get<file_id>(ctx), "FINISHED", "", false);
        }
}

