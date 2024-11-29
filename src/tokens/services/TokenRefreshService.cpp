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
#include "server/common/DrainMode.h"

#include "TokenRefreshService.h"
#include "tokens/executors/TokenRefreshExecutor.h"

using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;

namespace fts3 {
namespace token {


TokenRefreshService::TokenRefreshService(const std::shared_ptr<HeartBeat>& heartBeat) :
    BaseService("TokenRefreshService"), heartBeat(heartBeat)
{
    execPoolSize = ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("TokenRefreshCheckInterval");
    bulkSize = ServerConfig::instance().get<int>("TokenRefreshBulkSize");
}

void TokenRefreshService::runService()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefreshService interval: " << pollInterval.total_seconds() << "s" << commit;
    int refreshedTokensLastRun = 0;

    while (!boost::this_thread::interruption_requested()) {
        updateLastRunTimepoint();

        // Stand-by if the previous run did not max out the token-refresh bulk size
        if (refreshedTokensLastRun < bulkSize) {
            boost::this_thread::sleep(pollInterval);
        }

        // Reset value here in case unexpected error is thrown in the main try/catch
        refreshedTokensLastRun = 0;

        try {
            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-refresh for this instance!" << commit;
                continue;
            }

            // This service intentionally runs only on the first node
            if (heartBeat->isLeadNode(true)) {
                // Refresh access tokens that are marked for refresh
                refreshedTokensLastRun = refreshTokens();
                handleFailedTokenRefresh();
            }
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenRefreshService!" << commit;
            break;
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenRefreshService!" << commit;
        }
    }
}

int TokenRefreshService::refreshTokens()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    ThreadPool<TokenRefreshExecutor> execPool(execPoolSize);

    try {
        auto providers = db->getTokenProviders();
        auto tokens = db->getAccessTokensForRefresh(bulkSize);

        if (tokens.empty()) {
            return 0;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Retrieved " << tokens.size() << " tokens for token-refresh" << commit;

        for (auto& token: tokens) {
            if (boost::this_thread::interruption_requested()) {
                execPool.interrupt();
                return -1;
            }

            auto* exec = new TokenRefreshExecutor(token, providers[token.issuer], *this);
            execPool.start(exec);
        }

        // Wait for all the workers to finish
        execPool.join();

        {
            boost::unique_lock<boost::shared_mutex> lock(mxRefreshed);

            for (const auto& it: refreshedTokens) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Storing refreshed token: "
                                                << "token_id=" << it.tokenId << " "
                                                << "expiration_time=" << it.expiry << " "
                                                << "access_token=" << it.accessTokenToString() << " "
                                                << "refresh_token=" << it.refreshTokenToString()
                                                << commit;
            }

            if (!refreshedTokens.empty()) {
                db->storeRefreshedTokens(refreshedTokens);
                refreshedTokens.clear();
            }
        }

        return static_cast<int>(tokens.size());
    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Interruption requested in TokenRefreshService:refreshTokens" << commit;
        execPool.interrupt();
        execPool.join();
        throw;
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService:refreshTokens: " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenRefreshService:refreshTokens! " << commit;
    }

    return -1;
}

void TokenRefreshService::handleFailedTokenRefresh()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);

    for (const auto& it: failedRefreshes) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Handling failed token refresh: "
                                        << "token_id=" << it.first << " "
                                        << "message=\"" << it.second << "\""
                                        << commit;
    }

    if (!failedRefreshes.empty()) {
        db->markFailedTokenRefresh(failedRefreshes);
        failedRefreshes.clear();
    }
}

void TokenRefreshService::registerRefreshedToken(const RefreshedToken& refreshedToken)
{
    boost::unique_lock<boost::shared_mutex> lock(mxRefreshed);
    refreshedTokens.emplace(refreshedToken);
}

void TokenRefreshService::registerFailedTokenRefresh(const std::string& token_id, const std::string& message)
{
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);
    failedRefreshes.emplace(token_id, message);
}

} // end namespace token
} // end namespace fts3
