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

#include "common/Exceptions.h"
#include "common/Logger.h"

#include "QoSTransitionTask.h"
#include "CDMIPollTask.h"
#include "WaitingRoom.h"


void QoSTransitionTask::run(const boost::any &)
{
	bool anySuccessful = false;
    auto transitions = ctx.getSurls();

	for (auto it_t = transitions.begin(); it_t != transitions.end(); ++it_t) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Perform QoS transition of " << it_t->surl << " to QoS: " << it_t->target_qos << commit;

        // Perform transition
        try {
            gfal2QoS.changeFileQoS(it_t->surl, it_t->target_qos, it_t->token);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition Request of " << it_t->surl << " to "
                                            << it_t->target_qos << " submitted successfully" << commit;

            if (ctx.cdmiUpdateFileStateToQosRequestSubmitted(it_t->jobId, it_t->fileId)) {
                anySuccessful = true;
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Failed to update database QoS state for " << it_t->surl << " [" << it_t->target_qos << "]. "
                                                 << "Assuming other server updated the transition already." << commit;
            }
        } catch (Gfal2Exception &err) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition of " << it_t->surl << " to " << it_t->target_qos << " failed" << commit;
            ctx.cdmiUpdateFileStateToFailed(it_t->jobId, it_t->fileId, err.what());
        }
	}

	if (anySuccessful) {
		ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
	}
}
