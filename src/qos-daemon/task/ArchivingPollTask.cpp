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

std::set<std::pair<std::string, std::string>> ArchivingPollTask::active_urls;

void ArchivingPollTask::run(const boost::any&)
{
	if (timeout_occurred()) return;
	// handle cancelled jobs/files
	handle_canceled();

	//use the same var for staging pool retries now
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

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ArchivingPollTask starting"
        << " [ files=" << urls.size() << " / start=" <<  ctx.getStartTime()
        << " / timeout=" << ctx.getArchiveTimeout() << " ]" << commit;

	std::vector<GError*> errors(urls.size(), NULL);
	std::vector<const char*> failedUrls;

	int status = gfal2_archive_poll_list(gfal2_ctx, static_cast<int>(urls.size()), urls.data(), errors.data());

	// Status return code meaning:
	//  0  - Not all files are in terminal state
	//  1  - All files are in terminal successful state
	//  2  - All files are in terminal state, but not all are successful
	// -1  - All files are in error state

    for (size_t i = 0 ; i < urls.size(); i++) {
        auto ids = ctx.getIDs(urls[i]);

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
                    for (auto it = ids.begin(); it != ids.end(); ++it) {
                        ctx.updateState(it->first, it->second, "FAILED", JobError("ARCHIVING", errors[i]));
                    }
                    failedUrls.push_back(urls[i]);
                }
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING polling FAILED for " << urls[i] << ": "
                                                  << errors[i]->code << " " << errors[i]->message << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FAILED", JobError("ARCHIVING", errors[i]));
                }
                failedUrls.push_back(urls[i]);
            }
        } else {
            if (status >= 0) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING FINISHED for " << urls[i] << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
                ctx.removeUrl(urls[i]);
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING polling FAILED for " << urls[i] << ": "
                                                  << errors[i]->code << " " << errors[i]->message << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FAILED",
                                    JobError("ARCHIVING", -1, "Error not set by Gfal2"));
                }
                failedUrls.push_back(urls[i]);
            }
        }

        g_clear_error(&errors[i]);
    }

	// Schedule a new poll
	if (status == 0 || forcePoll) {
		time_t interval = getPollInterval(++nPolls);
		time_t now = time(NULL);
		wait_until = now + interval;

		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ARCHIVING polling " << ctx.getLogMsg() << commit;
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "ARCHIVING polling next attempt in " << interval << " seconds" << commit;
		ctx.getWaitingRoom().add(new ArchivingPollTask(std::move(*this)));
	}
}


void ArchivingPollTask::handle_canceled()
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
}


bool ArchivingPollTask::timeout_occurred()
{
	// first check if archive timeout was exceeded
	if (!ctx.hasTimeoutExpired()) return false;
	// get URLs
	std::set<std::string> urls = ctx.getUrls();
	// Log the event per file
	for (auto i = urls.begin(); i != urls.end(); ++i) {
		auto ids = ctx.getIDs(*i);
		for (auto j = ids.begin(); j != ids.end(); ++j) {
			FTS3_COMMON_LOGGER_NEWLOG(NOTICE) << "ARCHIVING timeout triggered for "
					<< j->first << "/" << j->second
					<< commit;
		}
	}

	// set the state
	ctx.updateState("FAILED",
			JobError("ARCHIVING", ETIMEDOUT, "archiving timeout has been exceeded")
	);
	// confirm the timeout
	return true;
}
