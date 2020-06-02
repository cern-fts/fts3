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
#include "CDMIPollTask.h"

void CDMIPollTask::run(const boost::any&)
{
	FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMIPollTask starting" << commit;

	std::map<std::string, CDMIQosTransitionContext> tasks;

	// Retrieve all files with a state of QOS_REQUEST_SUBMITTED
	std::vector<QosTransitionOperation> files;
	ctx.cdmiGetFilesForQosRequestSubmitted(files, "QOS_REQUEST_SUBMITTED");

	int maxPollRetries = fts3::config::ServerConfig::instance().get<int>("StagingPollRetries");
	bool finished = true;

	for (auto it_f = files.begin(); it_f != files.end(); ++it_f)
	{
		GError *err = NULL;

		// Add token to context
		gfal2_cred_t *cred = gfal2_cred_new(GFAL_CRED_BEARER, it_f->token.c_str());
		gfal2_cred_set(gfal2_ctx, Uri::parse(it_f->surl).host.c_str(), cred, &err);

		// Check QoS of file
		// TODO: add Gfal2 error checking
        const char* qos_result = gfal2_check_file_qos(gfal2_ctx, it_f->surl.c_str(), &err);

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "CDMI check QoS of file " << it_f->surl << ": "
                                         << "(requested_target_qos=" << it_f->target_qos.c_str() << ", actual_qos=" << qos_result << ")" << commit;

        if (qos_result != NULL && std::strcmp(it_f->target_qos.c_str(), qos_result) == 0) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << ": [Success] File was successfully transitioned" << commit;
            ctx.cdmiUpdateFileStateToFinished(it_f->jobId, it_f->fileId);
        } else {
            const char* qos_target_result = gfal2_check_target_qos(gfal2_ctx, it_f->surl.c_str(), &err);
            finished = false;

            if (qos_target_result != NULL && std::strcmp(it_f->target_qos.c_str(), qos_target_result) == 0) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << ": [Pending] File has not been transitioned yet" << commit;
            } else if (qos_target_result == NULL) {
                // No target QoS found --> transition might have finished
                qos_result = gfal2_check_file_qos(gfal2_ctx, it_f->surl.c_str(), &err);

                if (qos_result != NULL && std::strcmp(it_f->target_qos.c_str(), qos_result) == 0) {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << ": [Success] File was successfully transitioned" << commit;
                    ctx.cdmiUpdateFileStateToFinished(it_f->jobId, it_f->fileId);
                } else {
                    qos_result = (qos_result == NULL) ? "NULL" : qos_result;
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << ": [Failed] Transition has finished but did not reach target QoS "
                                                    << "(requested_target_qos=" << it_f->target_qos.c_str()
                                                    << ", actual_qos=" << qos_result << "). Marking file as FAILED " << commit;
                    ctx.cdmiUpdateFileStateToFailed(it_f->jobId, it_f->fileId);
                }
            } else if (std::strcmp(it_f->target_qos.c_str(), qos_target_result) != 0) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI check QoS of file " << it_f->surl << ": [Failed] Target QoS has changed to a different value "
                                                << "(requested_target_qos=" << it_f->target_qos.c_str()
                                                << ", actual_target_qos=" << qos_target_result << "). Marking file as FAILED" << commit;
                ctx.cdmiUpdateFileStateToFailed(it_f->jobId, it_f->fileId);
            }

            if (err && ctx.incrementErrorCountForSurl(it_f->surl) == maxPollRetries) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMI POLL check QoS of file " << it_f->surl << " exceeded the max configured limit retry."
                                                << " File has not been transitioned yet and is marked as FAILED" << commit;
                ctx.cdmiUpdateFileStateToFailed(it_f->jobId, it_f->fileId);
            }
        }

		// Delete token from context
		gfal2_cred_free(cred);
		gfal2_cred_clean(gfal2_ctx, &err);
	}

    // If not all transitions are finished, schedule a new poll
    if (!finished) {
        time_t interval = getPollInterval(++nPolls), now = time(NULL);
        wait_until = now + interval;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMIPollTask resolution: Next QoS check attempt in " << interval << " seconds" << commit;
        ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
    } else {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CDMIPollTask resolution: All tracked QoS transitions finished";
    }
}
