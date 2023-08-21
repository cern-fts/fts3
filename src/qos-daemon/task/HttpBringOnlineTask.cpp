/*
 * Copyright (c) CERN 2022
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

#include "HttpBringOnlineTask.h"
#include "HttpPollTask.h"
#include "WaitingRoom.h"


boost::shared_mutex HttpBringOnlineTask::mx;

std::set<std::pair<std::string, std::string>> HttpBringOnlineTask::active_urls;


void HttpBringOnlineTask::run(const boost::any &)
{
    char token[512] = {0};
    std::set<std::string> urlSet = ctx.getUrls();
    if (urlSet.empty())
        return;

    std::vector<const char*> urls;
    std::vector<const char*> metadata;
    std::vector<std::string> _metadata;
    urls.reserve(urlSet.size());
    metadata.reserve(urlSet.size());
    _metadata.reserve(urlSet.size());
    for (auto set_i = urlSet.begin(); set_i != urlSet.end(); ++set_i) {
        urls.emplace_back(set_i->c_str());
        _metadata.emplace_back(ctx.getMetadata(*set_i));
        metadata.emplace_back(_metadata.back().c_str());
    }

    std::vector<GError*> errors(urls.size(), NULL);

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE issuing bring-online for: " << urls.size() << " files"
                                    << " copy-pin-lifetime=" << ctx.getPinLifetime()
                                    << " bring-online-timeout=" << ctx.getBringonlineTimeout()
                                    << " storage=" << ctx.getStorageEndpoint() << commit;

    int status = gfal2_bring_online_list_v2(
                     gfal2_ctx,
                     static_cast<int>(urls.size()),
                     urls.data(),
                     metadata.data(),
                     ctx.getPinLifetime(),
                     ctx.getBringonlineTimeout(),
                     token,
                     sizeof(token),
                     1,
                     errors.data()
                 );

    if (status < 0) {
        for (size_t i = 0; i < urls.size(); ++i) {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i] && errors[i]->code != EOPNOTSUPP) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", errors[i])
                    );
                }
            }
            else if (errors[i] && errors[i]->code == EOPNOTSUPP)
            {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for "
                    << urls[i] << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "BRINGONLINE FAILED for " << urls[i] << ": returned -1 but error was not set "
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", -1, "Error not set by gfal2")
                    );
                }
            }
        }
    }
    else if (status == 0) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "BRINGONLINE queued, got token " << token << ": "
                                        << ctx.getLogMsg() << commit;

        ctx.updateState(token);
        ctx.getHttpWaitingRoom().add(new HttpPollTask(std::move(*this), token));
    }
    else {
        // No need to poll
        for (size_t i = 0; i < urls.size(); ++i)
        {
            auto ids = ctx.getIDs(urls[i]);

            if (errors[i] == NULL) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for "
                    << urls[i] << " , got token " << token
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
            }
            else if (errors[i]->code == EOPNOTSUPP) {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FINISHED for "
                    << urls[i]
                    << ": not supported, keep going (" << errors[i]->message << ")"
                    << commit;
                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second, "FINISHED", JobError());
                }
                ctx.removeUrl(urls[i]);
            }
            else
            {
                FTS3_COMMON_LOGGER_NEWLOG(NOTICE)
                    << "BRINGONLINE FAILED for " << urls[i] << ": "
                    << errors[i]->code << " " << errors[i]->message
                    << commit;

                for (auto it = ids.begin(); it != ids.end(); ++it) {
                    ctx.updateState(it->first, it->second,
                                    "FAILED", JobError("STAGING", errors[i])
                    );
                }
            }
        }
    }

    for (size_t i = 0; i < urls.size(); ++i) {
        g_clear_error(&errors[i]);
    }
}
