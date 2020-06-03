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


boost::shared_mutex QoSTransitionTask::mx;

std::set<std::tuple<std::string, std::string, std::string, std::string, uint64_t>> QoSTransitionTask::active_surls;

void QoSTransitionTask::run(const boost::any &)
{
	std::vector<GError*> errors(active_surls.size(), NULL);
	bool anySuccessful = false;

	for (auto it = active_surls.begin(); it != active_surls.end(); ++it) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Perform QoS transition of " << std::get<0>(*it) << " to QoS: " << std::get<2>(*it) << commit;

        // Perform transition
        try {
            gfal2QoS.changeFileQoS(std::get<0>(*it), std::get<2>(*it), std::get<1>(*it));
        } catch (Gfal2Exception &err) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition of " << std::get<0>(*it) << " to " << std::get<2>(*it) << " failed" << commit;
            ctx.cdmiUpdateFileStateToFailed(std::get<3>(*it), std::get<4>(*it));
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "QoS Transition Request of " << std::get<0>(*it) << " to " << std::get<2>(*it) << " submitted successfully" << commit;
        ctx.cdmiUpdateFileStateToQosRequestSubmitted(std::get<3>(*it), std::get<4>(*it));
        anySuccessful = true;
	}

	if (anySuccessful) {
		ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
	}
}
