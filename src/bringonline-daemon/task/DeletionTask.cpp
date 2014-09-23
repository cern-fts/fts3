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
    run_srm_impl();
    run_impl();
}

void DeletionTask::run_srm_impl()
{
    GError *error = NULL;

    std::vector<char const *> urls = ctx.getSrmUrls();
    // make sure there is work to do
    if (urls.empty()) return;
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
            ctx.state_update("FINISHED", "", false);
        }
}

void DeletionTask::run_impl()
{
    GError *error = NULL;
    int status = 0;

    std::map< std::string, std::vector<std::pair<int, std::string> > > const & urls = ctx.getGsiftpJobs();
    // make sure that there is work to be done
    if (urls.empty()) return;

    std::map< std::string, std::vector<std::pair<int, std::string> > >::const_iterator it_j;
    std::vector<std::pair<int, std::string> >::const_iterator it_f;

    // first carry out the unlinking and store the jobs in respective container accordingly to the result
    for (it_j = urls.begin(); it_j != urls.end(); ++it_j)
        {
            for (it_f = it_j->second.begin(); it_f != it_j->second.end(); ++it_f)
                {
                    char const * url = it_f->second.c_str();
                    status = gfal2_unlink(gfal2_ctx, url, &error);

                    if (status < 0)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION FAILED "
                                                           << it_j->first << " (" << it_f->first << ") "
                                                           << error->code << " "
                                                           << error->message  << commit;

                            bool retry = doRetry(error->code, "SOURCE", std::string(error->message));
                            ctx.state_update(it_j->first, it_f->first, "FAILED", error->message, retry);
                            g_clear_error(&error);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION FINISHED "
                                                            << it_j->first << " (" << it_f->first << ") " <<  commit;
                            ctx.state_update(it_j->first, it_f->first, "FINISHED", "", false);
                        }
                }
        }
}
