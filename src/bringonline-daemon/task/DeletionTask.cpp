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

#include "DeletionTask.h"
#include "WaitingRoom.h"

#include "common/error.h"

#include <unordered_map>
#include "../../common/Logger.h"

DeletionTask::DeletionTask(DeletionContext const & ctx) : Gfal2Task("DELETION"), ctx(ctx)
{
    // set the proxy certificate
    setProxy(ctx);
}

void DeletionTask::run(boost::any const &)
{
    run_impl();
}

void DeletionTask::run_impl()
{
    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    urls.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.push_back(set_i->c_str());
    }

    std::vector<GError*> error (urls.size(), NULL);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION Bulk of " << urls.size() << " files" << commit;

    int status = gfal2_unlink_list(gfal2_ctx, (int)urls.size(), urls.data(), error.data());

    if (status < 0)
        {
            for (size_t i = 0; i < urls.size(); ++i)
                {
                    std::vector< std::pair<std::string, int> > const ids = ctx.getIDs(urls[i]);

                    if (error[i])
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "DELETION FAILED " << urls[i] << ": "
                                                           << error[i]->code << " " << error[i]->message
                                                           << commit;

                            bool retry = doRetry(error[i]->code, "SOURCE", std::string(error[i]->message));
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FAILED", error[i]->message, retry);
                        }
                    else
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION FINISHED for " << urls[i] << commit;
                            for (auto it = ids.begin(); it != ids.end(); ++it)
                                ctx.state_update(it->first, it->second, "FINISHED", "", false);
                        }
                    g_clear_error(&error[i]);
                }
        }
    else
        {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION FINISHED "
                                            << ctx.getLogMsg() <<  commit;
            ctx.state_update("FINISHED", "", false);
        }
}
