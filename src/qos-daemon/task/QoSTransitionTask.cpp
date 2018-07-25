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
#include "PollTask.h"
#include "WaitingRoom.h"


boost::shared_mutex QoSTransitionTask::mx;

std::set<std::pair<std::string, std::string>> QoSTransitionTask::active_urls;


void QoSTransitionTask::run(const boost::any &)
{
	std::cout << "Inside the QoS transition task" << std::endl;
    /*char token[512] = {0};
    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    std::vector<GError*> errors(urls.size(), NULL);

    /////////////FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE issuing bring-online for: " << urls.size() << " files, "
    /////////////                                << "with copy-pin-lifetime: " << ctx.getPinlifetime() <<
    /////////////                                " and bring-online-timeout: " << ctx.getBringonlineTimeout() << commit;

    GError *err = NULL;
    for (size_t i = 0; i < urls.size(); ++i) {
    	int result = gfal2_change_object_qos(context, urls[i], "/cdmi_capabilities/dataobject/tape", &err);
    }
    g_clear_error(&err);*/

    //for (size_t i = 0; i < urls.size(); ++i) {
    //    g_clear_error(&errors[i]);
    //}
}
