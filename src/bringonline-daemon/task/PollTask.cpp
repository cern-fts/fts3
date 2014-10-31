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
    handle_canceled();
    handle_timeout();

    std::vector<char const *> urls = ctx.getUrls();
    // make sure there is work to be done
    if (urls.empty()) return;

    std::vector<GError*> errors(urls.size(), NULL);

    int status = gfal2_bring_online_poll_list(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), token.c_str(), errors.data());

    if (status < 0)
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    auto ids = ctx.getIDs(urls[i]);

                    if (errors[i] && errors[i]->code != EOPNOTSUPP)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE polling FAILED for " << urls[i] << ": "
                                                           << errors[i]->code << " " << errors[i]->message
                                                           << commit;

                            bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", errors[i]->message, retry);
                        }
                    else if (errors[i] && errors[i]->code == EOPNOTSUPP)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "BRINGONLINE FINISHED for " << urls[i]
                                                            << ": not supported, keep going (" << errors[i]->message << ")" << commit;
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FINISHED", "", false);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "BRINGONLINE FAILED for " << urls[i]
                                                            << ": returned -1 but error was not set ";
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", "Error not set by gfal2", false);
                        }
                    g_clear_error(&errors[i]);
                }
        }
    // 0 = not all are terminal
    // 1 = all are terminal
    // So check the status for each individual file regardless, so we can start transferring before the
    // whole staging job is finished
    else
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    auto ids = ctx.getIDs(urls[i]);

                    if (errors[i] == NULL)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED for "
                                                            << urls[i] << commit;
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FINISHED", "", false);
                            ctx.removeUrl(urls[i]);
                        }
                    else if (errors[i]->code == EAGAIN)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE NOT FINISHED for " << urls[i]
                                                            << ": " << errors[i]->message
                                                            << commit;
                        }
                    else if (errors[i]->code == EOPNOTSUPP)
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE FINISHED for "
                                                            << urls[i]
                                                            << ": not supported, keep going (" << errors[i]->message << ")" << commit;
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FINISHED", "", false);
                            ctx.removeUrl(urls[i]);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE FAILED for " << urls[i] << ": "
                                                           << errors[i]->code << " " << errors[i]->message
                                                           << commit;

                            bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                            for (auto it = ids.begin(); it != ids.end() ; ++it)
                                ctx.state_update(it->first, it->second, "FAILED", errors[i]->message, retry);
                            ctx.removeUrl(urls[i]);

                        }
                    g_clear_error(&errors[i]);
                }
        }

    // If status was 0, not everything is terminal, so schedule a new poll
    if(status == 0)
        {
            time_t interval = getPollInterval(++nPolls), now = time(NULL);
            wait_until = now + interval;
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling "
                                            << ctx.getLogMsg()
                                            << token << commit;

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
            WaitingRoom<PollTask>::instance().add(new PollTask(std::move(*this)));
        }
}

void PollTask::handle_canceled()
{
    std::vector<char const *> urls;
    std::set<std::string> remove;

    // critical section
    {
        boost::shared_lock<boost::shared_mutex> lock(mx);
        // first check if the token is still active
        auto it = active_tokens.find(token);
        if (it == active_tokens.end())
            {
                // if not abort all URLs
                urls = ctx.getUrls();
            }
        else
            {
                std::set<std::string> surls = ctx.getSurls();
                // otherwise check if some of the URLs should be aborted
                std::set_difference(
                    surls.begin(), surls.end(),
                    it->second.begin(), it->second.end(),
                    std::inserter(remove, remove.end())
                );
            }
    }
    // check if the whole task has been cancelled
    if (!urls.empty())
        {
            ctx.clear();
            abort(urls);
            return;
        }
    // check if there is something to do first
    if (remove.empty()) return;
    // make sure in the context are only those URLs that where not cancelled
    ctx.remove(remove);
    // if only a subset of URLs should be aborted ...
    urls.reserve(remove.size());
    // fill in with URLs
    for (auto it = remove.begin(); it != remove.end(); ++it)
        {
            urls.push_back(it->c_str());
        }
    abort(urls);
}

void PollTask::handle_timeout()
{
    // first check if bring-online timeout was exceeded
    if (!ctx.is_timeouted()) return;
    // set the state
    ctx.state_update("FAILED", "bring-online timeout has been exceeded", true);
    // get URLs
    auto urls = ctx.getUrls();
    ctx.clear();
    // and abort the bring-online operation
    abort(urls);
}

void PollTask::abort(std::vector<char const *> const & urls)
{
    if (urls.empty()) return;

    std::vector<GError*> errors(urls.size(), NULL);
    int status = gfal2_abort_files(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), token.c_str(), errors.data());

    if (status < 0)
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    auto ids = ctx.getIDs(urls[i]);

                    if (errors[i])
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE abort FAILED for " << urls[i] << ": "
                                                           << errors[i]->code << " " << errors[i]->message
                                                           << commit;

                            bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", errors[i]->message, retry);
                            g_clear_error(&errors[i]);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE abort FAILED for "
                                                            << urls[i] << ", returned -1, but error not set " << token
                                                            << commit;
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", "Error not set by gfal2", false);
                        }
                }
        }
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
