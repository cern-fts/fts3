/*
 * Copyright (c) CERN 2024
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

#include "common/Logger.h"
#include "common/DaemonTools.h"
#include "common/ThreadPool.h"

#include "config/ServerConfig.h"
#include "server/DrainMode.h"

#include "TokenRefreshService.h"
#include "TokenRefreshExecutor.h"

#include <cstdlib>

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace server {

extern time_t tokenRefreshRecords;


TokenRefreshService::TokenRefreshService(HeartBeat *beat) :
    BaseService("TokenRefreshService"), beat(beat)
{
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("TokenRefreshCheckInterval");
}

void TokenRefreshService::runService() {

    while (!boost::this_thread::interruption_requested()) {
        tokenRefreshRecords = time(nullptr);

        try {
            boost::this_thread::sleep(pollInterval);

            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-refresh for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            // This service intentionally runs only on the first node
            if (beat->isLeadNode(true)) {
                refreshAccessTokens();
                handleFailedTokenRefresh();
            }
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService!" << commit;
        }
    }
}

void TokenRefreshService::refreshAccessTokens() {
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    ThreadPool<TokenRefreshExecutor> execPool(execPoolSize);

    try {
        auto providers = db->getTokenProviders();

        time_t start = time(nullptr);
        auto tokens = db->getAccessTokensForRefresh();
        time_t end = time(nullptr);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TokenRefreshService\" "
                                        << "func=\"refreshAccessTokens\" "
                                        << "DBcall=\"getAccessTokensForRefresh\" "
                                        << "time=\"" << end - start << "\""
                                        << commit;

        if (tokens.empty()) {
            return;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Retrieved " << tokens.size() << " tokens for token-refresh" << commit;

        for (auto& token: tokens) {
            if (boost::this_thread::interruption_requested()) {
                execPool.interrupt();
                return;
            }

            TokenRefreshExecutor *exec =
                    new TokenRefreshExecutor(token, providers[token.issuer], *this);
            execPool.start(exec);
        }

        // Wait for all the workers to finish
        execPool.join();

        {
            boost::unique_lock<boost::shared_mutex> lock(mxRefresh);

            for (const auto& it: refreshedTokens) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Storing refreshed access token: "
                                                << "token_id=" << it.tokenId << " "
                                                << "access_token=" << it.accessTokenToString() << " "
                                                << "refresh_token=" << it.refreshTokenToString()
                                                << commit;
            }

            if (!refreshedTokens.empty()) {
                db->storeRefreshedAccessTokens(refreshedTokens);
                refreshedTokens.clear();
            }
        }
    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TokenRefreshService:refreshAccessTokens" << commit;
        execPool.interrupt();
        execPool.join();
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService:refreshAccessTokens " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService! " << commit;
    }
}

void TokenRefreshService::handleFailedTokenRefresh()
{
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);

    for (const auto& it: failedRefreshes) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Handling failed token refresh: "
                                        << "token_id=" << it.first << " "
                                        << "message=" << it.second
                                        << commit;
    }

    if (!failedRefreshes.empty()) {
        // TODO: Store token-refresh failure reason in the database,
        //       as well as retry timestamp to prevent retrying too often
        // db->markFailedTokenRefresh(...)
        failedRefreshes.clear();
    }
}

void TokenRefreshService::registerRefreshedAccessToken(const RefreshedToken& refreshedToken)
{
    boost::unique_lock<boost::shared_mutex> lock(mxRefresh);
    refreshedTokens.insert(refreshedToken);
}

void TokenRefreshService::registerFailedTokenRefresh(const std::string& token_id, const std::string& message)
{
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);
    failedRefreshes.emplace(token_id, message);
}

} // end namespace server
} // end namespace fts3
