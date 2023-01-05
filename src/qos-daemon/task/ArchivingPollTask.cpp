/*
 * Copyright (c) CERN 2013-2020
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

#include "ArchivingPollTask.h"

boost::shared_mutex ArchivingPollTask::mx;

std::set<std::tuple<std::string, std::string, uint64_t>> ArchivingPollTask::active_urls;

/// Steps taken by the ArchivingPollTask:
///   1. Iterate transfers once and remove timeouts
///   2. Flatten the map to obtain a set of sURLs
///   3. Remove the sURLs presented by gfal2_archive_poll_list as finished
void ArchivingPollTask::run(const boost::any&)
{
    // Check if archiving timeout has expired for some jobs
    handle_timeouts();
    // Check if some jobs were canceled
    handle_canceled();

	int maxPollRetries = ctx.getMaxPollRetries();
	bool forcePoll = false;

	std::set<std::string> urlSet = ctx.getUrls();
	if (urlSet.empty())
		return;

	std::vector<const char*> urls;
	urls.reserve(urlSet.size());
	for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
		urls.push_back(set_i->c_str());
	}

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ArchivingPollTask starting"
        << " [ files=" << urls.size() << " / start=" <<  ctx.getStartTime()
        << " / storage=" << ctx.getStorageEndpoint() << " ]"
        << commit;

	std::vector<GError*> errors(urls.size(), NULL);

    time_t start = time(0);
    int status = gfal2_archive_poll_list(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), errors.data());
    time_t end = time(0);
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Gfal2time=\"ArchivingPollTask\" "
                                    << "func=\"run\" "
                                    << "Gfal2call=\"gfal2_archive_poll_list\" "
                                    << "time=\"" << end - start << "\""
                                    << commit;

	// Status return code meaning:
	//  0  - Not all files are in terminal state
	//  1  - All files are in terminal successful state
	//  2  - All files are in terminal state, but not all are successful
	// -1  - All files are in error state

    for (size_t i = 0 ; i < urls.size(); i++) {

        if (errors[i]) {
            if (errors[i]->code == ECOMM && ctx.incrementErrorCountForSurl(urls[i]) < maxPollRetries) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING ONGOING for " << urls[i] << "."
                                                  << " Communication error, soft failure: " << errors[i]->message
                                                  << commit;
                forcePoll = true;
            } else if (errors[i]->code == EAGAIN) {
                if (status == 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING ONGOING for " << urls[i] << commit;
                } else {
                    FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING polling FAILED for " << urls[i] << "."
                                                       << " EAGAIN error code not expected in terminal state: "
                                                       << errors[i]->message << commit;

                    set_terminal_state(urls[i], "FAILED", JobError("ARCHIVING", errors[i]));
                }
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING polling FAILED for " << urls[i] << ": "
                                                  << errors[i]->code << " " << errors[i]->message << commit;
                set_terminal_state(urls[i], "FAILED", JobError("ARCHIVING", errors[i]));
            }
        } else {
            if (status >= 0) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING FINISHED for " << urls[i] << commit;
                set_terminal_state(urls[i], "FINISHED", JobError());
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING polling FAILED for " << urls[i] << ": "
                                                  << errors[i]->code << " " << errors[i]->message << commit;
                set_terminal_state(urls[i], "FAILED", JobError("ARCHIVING", -1, "Error not set by Gfal2"));
            }
        }

        g_clear_error(&errors[i]);
    }

    // Schedule a new poll
    if (status == 0 || forcePoll) {
        time_t interval = ctx.getPollInterval();
        time_t now = time(NULL);
        wait_until = now + interval;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ARCHIVING polling " << ctx.getLogMsg() << commit;
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ARCHIVING polling next attempt in " << interval << " seconds" << commit;
        ctx.getWaitingRoom().add(new ArchivingPollTask(std::move(*this)));
    }
}


void ArchivingPollTask::handle_timeouts()
{
    std::set<std::tuple<std::string, std::string, uint64_t>> timeout_transfers;

    for (auto it = ctx.urlToIDs.begin(); it != ctx.urlToIDs.end(); it++) {
        const auto& job_id = it->second.first;
        const auto& file_id = it->second.second;

        if (ctx.hasTransferTimedOut(file_id)) {
            ctx.updateState(job_id, file_id, "FAILED",
                            JobError("ARCHIVING", ETIMEDOUT, "archive timeout has been exceeded"));
            timeout_transfers.emplace(it->first, job_id, file_id);
        }
    }

    if (!timeout_transfers.empty()) {
        ctx.removeTransfers(timeout_transfers);
        cancel(timeout_transfers);
    }
}


void ArchivingPollTask::set_terminal_state(const std::string& url, const std::string& state, const JobError& error)
{
    // Set of tuples <url, job_id, file_id> to remove from active urls
    std::set<std::tuple<std::string, std::string, uint64_t>> to_remove;

    auto range = ctx.urlToIDs.equal_range(url);

    for (auto it = range.first; it != range.second; ++it) {
        const auto& job_id = it->second.first;
        const auto& file_id = it->second.second;
        ctx.updateState(job_id, file_id, state, error);
        to_remove.emplace(url, job_id, file_id);
    }

    if (!to_remove.empty()) {
        ctx.removeTransfers(to_remove);
        cancel(to_remove);
    }
}


void ArchivingPollTask::handle_canceled()
{
    std::set<std::tuple<std::string, std::string, uint64_t>> to_remove;
    {
        // critical section
        boost::shared_lock<boost::shared_mutex> lock(mx);
        // get the URLs for the given task
        auto surls = ctx.getJobs();
        // check if some of the URLs should be aborted
        std::set_difference(
                surls.begin(), surls.end(),
                active_urls.begin(), active_urls.end(),
                std::inserter(to_remove, to_remove.end())
        );
    }
    // check if there is something to do first
    if (to_remove.empty()) return;
    // remove set of tuples from all the internal data structures
    ctx.removeTransfers(to_remove);
}
