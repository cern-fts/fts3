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

//#include "BringOnlineTask.h"
#include "CDMIPollTask.h"


void CDMIPollTask::run(const boost::any&)
{
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMIPollTask starting" << commit;

	std::map<std::string, CDMIQosTransitionContext> tasks;

	//Retrieve all files with a state of QOS_REQUEST_SUBMITTED
	std::vector<QosTransitionOperation> files;
	ctx.cdmiGetFilesForQosRequestSubmitted(files, "QOS_REQUEST_SUBMITTED");

	int maxPollRetries = fts3::config::ServerConfig::instance().get<int>("StagingPollRetries");

	bool anyFailed = false;
	for (auto it_f = files.begin(); it_f != files.end(); ++it_f)
	{
		GError *err = NULL;

		// Add token to context
		//std::cerr << "Printing token: " << it_f->token << std::endl;
		//std::cerr << "Printing host: " << Uri::parse(it_f->surl).host << std::endl;
		//std::cerr << "Printing surl: " << it_f->surl << std::endl;
		gfal2_cred_t *cred = gfal2_cred_new(GFAL_CRED_BEARER, it_f->token.c_str());
		gfal2_cred_set(gfal2_ctx, Uri::parse(it_f->surl).host.c_str(), cred, &err);

		// Check QoS of file
		// TODO: add error checking
		FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << commit;
		const char* qos_target_result = gfal2_check_target_qos(gfal2_ctx, it_f->surl.c_str(), &err);

		if (qos_target_result != NULL && std::strcmp(it_f->target_qos.c_str(),qos_target_result)==0) {
			FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << " was successful. File was successfully transitioned" << commit;
			// Update file state as finished
			ctx.cdmiUpdateFileStateToFinished(it_f->jobId, it_f->fileId);
		} else {
			FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << " failed. File has not been transitioned yet" << commit;
			anyFailed = true;
			if (err && ctx.incrementErrorCountForSurl(it_f->surl) == maxPollRetries) {
				ctx.cdmiUpdateFileStateToFailed(it_f->jobId, it_f->fileId);
				FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI POLL check QoS of file " << it_f->surl << " exceeded the max configured limit retry. File has not been transitioned yet and is marked as FAILED" << commit;
			}
		}

		//Delete token from context
		gfal2_cred_free(cred);
		gfal2_cred_clean(gfal2_ctx, &err);
	}

    // If they are not the same, schedule a new poll
    if (anyFailed) {
        time_t interval = getPollInterval(++nPolls), now = time(NULL);
        wait_until = now + interval;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << " Next QoS check attempt in " << interval << " seconds" << commit;
        ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
    }
}

