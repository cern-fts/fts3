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
    // check if the timeout was exceeded
    //if (timeout_occurred())
    //	return;
	std::cerr << "About to poll some stuff, inside the run" << std::endl;
    /*int maxPollRetries = fts3::config::ServerConfig::instance().get<int>("StagingPollRetries");
    bool forcePoll = false;

    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    std::vector<const char*> failedUrls;

    // This is wrong, test only with one for now
    GError *err = NULL;
    const char* qos_result = NULL;
    for (size_t i = 0; i < urls.size(); ++i) {
    	qos_target_result = gfal2_check_target_qos(gfal2_ctx, urls[i], &err);
    }


    // If they are not the same, schedule a new poll
    if (qos_target_result != NULL) {
        time_t interval = getPollInterval(++nPolls), now = time(NULL);
        wait_until = now + interval;
        FTS3_COMMON_LOGGER_NEWLOG(INFO)
            << "CDMIQoS polling " << ctx.getLogMsg() << token << commit;

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << " next attempt in " << interval << " seconds" << commit;
        ctx.getWaitingRoom().add(new CDMIPollTask(std::move(*this)));
    }*/
}

