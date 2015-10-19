/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gfal_api.h>
#include "common/Logger.h"

#include "BringOnlineTask.h"
#include "PollTask.h"
#include "WaitingRoom.h"


void PollTask::run(const boost::any&)
{
    // check if the bring-online timeout was exceeded
    if (timeout_occurred()) return;
    // handle cancelled jobs/files
    handle_canceled();

    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    std::vector<GError*> errors(urls.size(), NULL);

    int status = gfal2_bring_online_poll_list(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), token.c_str(), errors.data());

    if (status < 0) {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i] && errors[i]->code != EOPNOTSUPP) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "BRINGONLINE polling FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FAILED", errors[i]->message, retry);
                }
            }
            else if (errors[i] && errors[i]->code == EOPNOTSUPP)
            {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                    << "BRINGONLINE FINISHED for " << urls[i]
                    << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                 for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", "", false);
                 }
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                    << "BRINGONLINE FAILED for " << urls[i]
                    << ": returned -1 but error was not set "
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FAILED", "Error not set by gfal2", false);
                }
            }
            g_clear_error(&errors[i]);
        }
    }
    // 0 = not all are terminal
    // 1 = all are terminal
    // So check the status for each individual file regardless, so we can start transferring before the
    // whole staging job is finished
    else {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i] == NULL) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                    << "BRINGONLINE FINISHED for "
                    << urls[i]
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", "", false);
                }
                ctx.removeUrl(urls[i]);
            }
            else if (errors[i]->code == EAGAIN)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                    << "BRINGONLINE NOT FINISHED for " << urls[i]
                    << ": " << errors[i]->message
                    << commit;
            }
            else if (errors[i]->code == EOPNOTSUPP)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                    << "BRINGONLINE FINISHED for "
                    << urls[i]
                    << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", "", false);
                }
                ctx.removeUrl(urls[i]);
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "BRINGONLINE FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FAILED", errors[i]->message, retry);
                }
                ctx.removeUrl(urls[i]);

            }
            g_clear_error(&errors[i]);
        }
    }

    // If status was 0, not everything is terminal, so schedule a new poll
    if (status == 0) {
        time_t interval = getPollInterval(++nPolls), now = time(NULL);
        wait_until = now + interval;
        FTS3_COMMON_LOGGER_NEWLOG(INFO)
            << "BRINGONLINE polling " << ctx.getLogMsg() << token << commit;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
        WaitingRoom<PollTask>::instance().add(new PollTask(std::move(*this)));
    }
}


void PollTask::handle_canceled()
{
    std::set<std::pair<std::string, std::string>> remove;
    // critical section
    {
        boost::shared_lock<boost::shared_mutex> lock(mx);
        // get the URLs for the given task
        auto surls = ctx.getSurls();
        // check if some of the URLs should be aborted
        std::set_difference(
            surls.begin(), surls.end(),
            active_urls.begin(), active_urls.end(),
            std::inserter(remove, remove.end())
        );
    }
    // check if there is something to do first
    if (remove.empty()) return;
    // get the urls for abortions
    auto urls = ctx.getSurlsToAbort(remove);
    abort(urls);
}


bool PollTask::timeout_occurred()
{
    // first check if bring-online timeout was exceeded
    if (!ctx.hasTimeoutExpired()) return false;
    // get URLs
    auto urls = ctx.getUrls();
    // and abort the bring-online operation
    abort(urls, false);
    // set the state
    ctx.updateState("FAILED", "bring-online timeout has been exceeded", true);
    // confirm the timeout
    return true;
}


void PollTask::abort(std::set<std::string> const & urlSet, bool report)
{
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    std::vector<GError*> errors(urls.size(), NULL);
    int status = gfal2_abort_files(
        gfal2_ctx, static_cast<int>(urls.size()), urls.data(),
        token.c_str(), errors.data()
    );

    if (status < 0) {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i]) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "BRINGONLINE abort FAILED for " << urls[i] << ": "
                << errors[i]->code << " " << errors[i]->message
                << commit;
                if (report)
                {
                    bool retry = doRetry(errors[i]->code, "SOURCE", std::string(errors[i]->message));
                    for (auto it = ids.begin(); it != ids.end(); ++it)
                    ctx.updateState(it->first, it->second, "FAILED", errors[i]->message, retry);
                }
                g_clear_error(&errors[i]);
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE abort FAILED for "
                << urls[i] << ", returned -1, but error not set " << token
                << commit;
                if (report)
                {
                    for (auto it = ids.begin(); it != ids.end(); ++it)
                    ctx.updateState(it->first, it->second, "FAILED", "Error not set by gfal2", false);
                }
            }
        }
    }
}
