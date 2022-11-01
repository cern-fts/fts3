/*
 * Copyright (c) CERN 2022
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
#include "HttpPollTask.h"


void HttpPollTask::run(const boost::any&)
{
    // check if the bring-online timeout was exceeded
    if (timeout_occurred()) return;
    // handle cancelled jobs/files
    handle_canceled();

    int maxPollRetries = fts3::config::ServerConfig::instance().get<int>("StagingPollRetries");
    bool forcePoll = false;

    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BringonlinePollTask starting"
        << " [ files=" << urls.size() << " / token=" << token
        << " / minStagingStartTime=" << ctx.getStartTime()
        << " / maxBringonlineTimeout=" << ctx.getBringonlineTimeout()
        << " / storage=" << ctx.getStorageEndpoint() << " ]"
        << commit;

    std::vector<GError*> errors(urls.size(), NULL);
    std::vector<const char*> failedUrls;

    int status = gfal2_bring_online_poll_list(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), token.c_str(), errors.data());

    if (status < 0) {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i] && errors[i]->code == ECOMM && ctx.incrementErrorCountForSurl(urls[i]) < maxPollRetries) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE NOT FINISHED for " << urls[i]
                    << ". Communication error, soft failure: " << errors[i]->message
                    << commit;
                forcePoll = true;
            }
            else if (errors[i] && errors[i]->code != EOPNOTSUPP) {
                failedUrls.push_back(urls[i]);

                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE polling FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", errors[i])
                    );
                }
            }
            else if (errors[i] && errors[i]->code == EOPNOTSUPP)
            {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for " << urls[i]
                    << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FINISHED", JobError()
                    );
                }
            }
            else
            {
                failedUrls.push_back(urls[i]);

                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "BRINGONLINE FAILED for " << urls[i]
                    << ": returned -1 but error was not set "
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", -1, "Error not set by gfal2")
                    );
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
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for "
                    << urls[i]
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
                ctx.removeUrl(urls[i]);
            }
            else if (errors[i]->code == EAGAIN)
            {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE NOT FINISHED for " << urls[i]
                    << ": " << errors[i]->message
                    << commit;
            }
            else if (errors[i] && errors[i]->code == ECOMM && ctx.incrementErrorCountForSurl(urls[i]) < maxPollRetries) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE NOT FINISHED for " << urls[i]
                    << ". Communication error, soft failure: " << errors[i]->message
                    << commit;
                forcePoll = true;
            }
            else if (errors[i]->code == EOPNOTSUPP)
            {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for "
                    << urls[i]
                    << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
                ctx.removeUrl(urls[i]);
            }
            else
            {
                failedUrls.push_back(urls[i]);

                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", errors[i])
                    );
                }
                ctx.removeUrl(urls[i]);

            }
            g_clear_error(&errors[i]);
        }
    }

    // Issue a preventive abort for those that failed
    // For instance, Castor may give a failure saying the timeout expired, but the request will remain
    // on the queue until we explicitly cancel them.
    if (!failedUrls.empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE issuing an abort for staging requests that failed"
                                        << commit;
        std::vector<GError*> abortErrors(failedUrls.size(), NULL);
        int abort_status = gfal2_abort_files(
            gfal2_ctx, static_cast<int>(failedUrls.size()), failedUrls.data(),
            token.c_str(), abortErrors.data()
        );
        // Only log errors for postmortem, do not mark anything on the database
        if (abort_status < 0) {
            for (auto i = abortErrors.begin(); i != abortErrors.end(); ++i) {
                if (*i != NULL) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE got an error from the staging abort "
                                                    << (*i)->code << " " << (*i)->message
                                                    << commit;
                    g_clear_error(&(*i));
                }
            }
        }
    }

    // If status was 0, not everything is terminal, so schedule a new poll
    if (status == 0 || forcePoll) {
        time_t interval = getPollInterval(++nPolls);
        time_t now = time(NULL);
        wait_until = now + interval;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE polling " << ctx.getLogMsg() << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE next attempt in " << interval << " seconds" << commit;
        ctx.getHttpWaitingRoom().add(new HttpPollTask(std::move(*this)));
    }
}


void HttpPollTask::handle_canceled()
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


bool HttpPollTask::timeout_occurred()
{
    // first check if bring-online timeout was exceeded
    if (!ctx.hasTimeoutExpired()) return false;
    // get URLs
    std::set<std::string> urls = ctx.getUrls();
    // Log the event per file
    for (auto i = urls.begin(); i != urls.end(); ++i) {
        auto ids = ctx.getIDs(*i);
        for (auto j = ids.begin(); j != ids.end(); ++j) {
            FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "BRINGONLINE timeout triggered for "
                                              << j->first << "/" << j->second
                                              << commit;
        }
    }
    // and abort the bring-online operation
    abort(urls, false);
    // set the state
    ctx.updateState("FAILED",
                    JobError("STAGING", ETIMEDOUT, "bring-online timeout has been exceeded")
    );
    // confirm the timeout
    return true;
}


void HttpPollTask::abort(std::set<std::string> const & urlSet, bool report)
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

    if (status == 0) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE task canceled with token " << token << commit;
    }
    else {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i]) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "BRINGONLINE abort FAILED for " << urls[i] << ": "
                                                  << errors[i]->code << " " << errors[i]->message
                                                  << commit;
                if (report)
                {
                    for (auto it = ids.begin(); it != ids.end(); ++it) {
                        ctx.updateState(it->first, it->second,
                                        "FAILED", JobError("STAGING", errors[i])
                        );
                    }
                }
                g_clear_error(&errors[i]);
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE abort FAILED for "
                                               << urls[i] << ", returned -1, but error not set " << token
                                               << commit;
                if (report)
                {
                    for (auto it = ids.begin(); it != ids.end(); ++it)
                        ctx.updateState(it->first, it->second,
                                        "FAILED", JobError("STAGING", -1, "Error not set by gfal2")
                        );
                }
            }
        }
    }
}
