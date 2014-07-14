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

std::unordered_map< std::string, std::set<std::string> > PollTask::active_tokens;

void PollTask::run(boost::any const &)
{
    if (abort()) return;

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
            state_update(ctx.jobs, "FAILED", error->message, retry);
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

            state_update(ctx.jobs, "FINISHED", "", false);
        }
}

bool PollTask::abort()
{
    bool aborted = false;

    std::vector<char const *> urls;
    std::set<std::string> remove;
    std::unordered_map< std::string, std::set<std::string> >::const_iterator it;

    // critical section
    {
        boost::shared_lock<boost::shared_mutex> lock(mx);
        // first check if the token is still active
        it = active_tokens.find(token);
        if (it == active_tokens.end())
            {
                // if not abort all URLs
                aborted = true;
                urls = ctx.getUrls();
            }
        else
            {
                // otherwise check if some of the URLs should be aborted
                std::set_difference(
                    ctx.surls.begin(), ctx.surls.end(),
                    it->second.begin(), it->second.end(),
                    std::inserter(remove, remove.end())
                );
            }
    }

    // check if there is something to do first
    if (urls.empty() && remove.empty()) return aborted;

    // if only a subset of URLs should be aborted ...
    if (urls.empty())
        {
            // adjust the size
            urls.reserve(remove.size());
            // fill in with URLs
            std::set<std::string>::const_iterator it;
            for (it = remove.begin(); it != remove.end(); ++it)
                {
                    urls.push_back(it->c_str());
                }
            // make sure in the context are only those URLs that where not cancelled
            std::set<std::string> not_cancelled;
            std::set_difference(
                ctx.surls.begin(), ctx.surls.end(),
                remove.begin(), remove.end(),
                std::inserter(not_cancelled, not_cancelled.end())
            );
            ctx.surls.swap(not_cancelled);
        }

    GError * err;
    int status = gfal2_abort_files(gfal2_ctx, urls.size(), &*urls.begin(), token.c_str(), &err);

    if (status < 0)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE abort FAILED "
                                           << ctx.getLogMsg()
                                           << err->code << " " << err->message  << commit;
            g_clear_error(&err);
        }

    return aborted;
}

void PollTask::cancel(std::unordered_map< std::string, std::set<std::string> > const & remove)
{
    boost::unique_lock<boost::shared_mutex> lock(mx);

    std::unordered_map< std::string, std::set<std::string> >::const_iterator r_it;
    std::unordered_map< std::string, std::set<std::string> >::iterator at_it;

    for (r_it = remove.begin(); r_it != remove.end(); ++r_it)
        {
            // check if the token is still in active tokens
            at_it = active_tokens.find(r_it->first);
            if (at_it == active_tokens.end()) continue;

            std::set<std::string> result;
            std::set_difference(
                at_it->second.begin(), at_it->second.end(),
                r_it->second.begin(), r_it->second.end(),
                std::inserter(result, result.end())
            );

            if (result.empty())
                {
                    // if there aren't any files left remove the token
                    active_tokens.erase(at_it);
                }
            else
                {
                    // otherwise, swap to those that are left
                    at_it->second.swap(result);
                }
        }
}
