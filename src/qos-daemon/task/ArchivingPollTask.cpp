/*
 * Copyright (c) CERN 2013-2019
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

#include "ArchivingTask.h"
#include "ArchivingPollTask.h"


void ArchivingPollTask::run(const boost::any&)
{
	// check if the archive  timeout was exceeded
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

	std::vector<GError*> errors(urls.size(), NULL);
	std::vector<const char*> failedUrls;

	for (size_t i = 0; i < urls.size(); ++i) {
		auto ids = ctx.getIDs(urls[i]);
		char buffer[1024];

		ssize_t ret = gfal2_getxattr(gfal2_ctx,  urls[i], GFAL_XATTR_STATUS, buffer, sizeof(buffer), &errors[i]);

	    //TODO: if files are pooled for the first time we need to set the archive_start_time

		//check for errors
		if (ret > 0 and strlen(buffer) > 0 and errors[i] == 0) {
			bool found = false;
			int i = 0;
			//check for NEARLINE or ONLINE_AND_NEARLINE
			while (i < ret) {
				if (strncmp(buffer + i, GFAL_XATTR_STATUS_NEARLINE, sizeof(GFAL_XATTR_STATUS_NEARLINE)) == 0) {
					found = true;
					break;
				}
				if (strncmp(buffer + i, GFAL_XATTR_STATUS_NEARLINE_ONLINE, sizeof(GFAL_XATTR_STATUS_NEARLINE_ONLINE)) == 0) {
					found = true;
					break;
				}
				i += strlen(buffer + i) + 1;
			}
			if (found) {

				FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    		<< "ARCHIVING FINISHED for "
							<< urls[i]
									<< commit;
				//update the state of the file to finished
				for (auto it = ids.begin(); it != ids.end(); ++it) {
					ctx.updateState(it->first, it->second, "FINISHED", JobError());
				}
				ctx.removeUrl(urls[i]);
				//TODO: when completed we need to set the archive_finish_time

			} else {
				forcePoll = true;
			}
		}
		else if (errors[i] && errors[i]->code == ECOMM && ctx.incrementErrorCountForSurl(urls[i]) < maxPollRetries) {
			FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    		<< "ARCHIVING  NOT FINISHED for " << urls[i]
																	  << ". Communication error, soft failure: " << errors[i]->message
																	  << commit;
			forcePoll = true;
		}
		else if (errors[i]) {
			failedUrls.push_back(urls[i]);

			FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
			<< "ARCHIVING polling FAILED for " << urls[i] << ": "
			<< errors[i]->code << " " << errors[i]->message
			<< commit;

			for (auto it = ids.begin(); it != ids.end(); ++it) {
				ctx.updateState(it->first, it->second,
						"FAILED", JobError("ARCHIVING", errors[i])
				);
			}
		}
		else
		{
			failedUrls.push_back(urls[i]);

			FTS3_COMMON_LOGGER_NEWLOG(ERR)
			<< "ARCHIVING FAILED for " << urls[i]
											   << ": returned -1 but error was not set "
											   << commit;
			for (auto it = ids.begin(); it != ids.end(); ++it) {
				ctx.updateState(it->first, it->second,
						"FAILED", JobError("ARCHIVING", -1, "Error not set by gfal2")
				);
			}
		}
		g_clear_error(&errors[i]);
	}

	// If status was 0, not everything is terminal, so schedule a new poll
	if (forcePoll) {
		time_t interval = getPollInterval(++nPolls), now = time(NULL);
		wait_until = now + interval;
		FTS3_COMMON_LOGGER_NEWLOG(INFO)
		<< "ARCHIVING polling " << ctx.getLogMsg() << token << commit;

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


