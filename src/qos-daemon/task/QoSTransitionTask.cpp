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

	for (const auto& transition: transitions) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Perform QoS transition of " << transition.surl << " to QoS: " << transition.target_qos << commit;

        // Perform transition
        try {
            gfal2QoS.changeFileQoS(transition.surl, transition.target_qos, transition.token);
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition Request of " << transition.surl << " to "
                                            << transition.target_qos << " submitted successfully" << commit;

            if (ctx.cdmiUpdateFileStateToQosRequestSubmitted(transition.jobId, transition.fileId)) {
                anySuccessful = true;
            } else {
                FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Failed to update database QoS state for " << transition.surl << " [" << transition.target_qos << "]. "
                                                 << "Assuming other server updated the transition already." << commit;
            }
        } catch (Gfal2Exception &err) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition of " << transition.surl << " to " << transition.target_qos << " failed" << commit;
            ctx.cdmiUpdateFileStateToFailed(transition.jobId, transition.fileId);
        }
	}

	if (anySuccessful) {
		ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
	}
}
