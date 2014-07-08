/*
 * CancelStagingTask.cpp
 *
 *  Created on: 2 Jul 2014
 *      Author: simonm
 */

#include "CancelStagingTask.h"

void CancelStagingTask::run(boost::any const &)
{
	char const * token = boost::get<2>(ctx).c_str();
	char const * url = boost::get<1>(ctx).c_str();

	GError * err;
	int status = gfal2_abort_files(gfal2_ctx, 1, &url, token, &err);

	if (status < 0)
	{
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE abort FAILED "
                                       << boost::get<0>(ctx) << " "
                                       << boost::get<1>(ctx) <<  " "
                                       << boost::get<2>(ctx) <<  " "
                                       << err->code << " " << err->message  << commit;
        g_clear_error(&err);
	}
}
