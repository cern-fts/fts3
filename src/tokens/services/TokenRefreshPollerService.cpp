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

#include <ranges>

#include "common/Logger.h"
#include "common/TimeUtils.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "TokenRefreshPollerService.h"

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace token {


TokenRefreshPollerService::TokenRefreshPollerService() : BaseService("TokenRefreshPollerService") {}

void TokenRefreshPollerService::runService()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();

    while (!boost::this_thread::interruption_requested()) {
        try {
            updateLastRunTimepoint();
            boost::this_thread::sleep(boost::posix_time::seconds(15));

            std::list<std::string> tokensToRefreshMark;
            std::list<std::string> tokensToCacheClean;
            size_t tokensToPollInitialSize;
            size_t validAccessTokensSize;
            size_t failedATRefreshesSize;

            {
                boost::unique_lock<boost::shared_mutex> lockQuery(mxPoll);

                if (tokensToPoll.empty()) {
                   continue;
                }

                tokensToPollInitialSize = tokensToPoll.size();

                // Look-up the tokens-to-refresh-poll set in the database
                std::list<std::string> tokenIds;
                std::ranges::copy(std::views::keys(tokensToPoll), std::back_inserter(tokenIds));

                auto validAccessTokensDB = db->getValidAccessTokens(tokenIds);
                validAccessTokensSize = validAccessTokensDB.size();

                // Make the refreshed tokens available to the TokenRefreshListener service
                {
                    boost::unique_lock<boost::shared_mutex> lockResults(mxRefreshed);
                    auto validTokenIds = std::views::keys(validAccessTokensDB);
                    auto validTokens = std::views::values(validAccessTokensDB);
                    refreshedTokens.insert(validTokens.begin(), validTokens.end());
                    // Remove valid access tokens from the tokens-to-refresh-poll set
                    // and the marked-for-refresh cache
                    removeFromContainer(tokensToPoll, validTokenIds);
                    removeFromContainer(markedTokensCache, validTokenIds);
                }

                // Look-up the remaining tokens-to-refresh-poll set for token-refresh failures in the database
                auto failedRefreshesDB = db->getFailedAccessTokenRefreshes(tokenIds);

                // Make the failed token-refreshes available to the TokenRefreshListener service
                {
                    boost::unique_lock<boost::shared_mutex> lockResults(mxFailed);
                    std::list<std::string> failedTokenIds;

                    for (const auto& [token_id, msgPair]: failedRefreshesDB) {
                        const auto& it_poll = tokensToPoll.find(token_id);

                        if (it_poll->second < msgPair.second) {
                            failedRefreshes.emplace(token_id, msgPair);
                            failedTokenIds.emplace_back(token_id);
                        }
                    }

                    // Remove failed access token refreshes from the tokens-to-refresh-poll set
                    // and the marked-for-refresh cache
                    removeFromContainer(tokensToPoll, failedTokenIds);
                    removeFromContainer(markedTokensCache, failedTokenIds);
                    failedATRefreshesSize = failedTokenIds.size();
                }

                // Mark the remaining tokens-to-refresh-poll in the database for refreshing
                // Note: we use an in-memory cache to prevent excessive database marking
                for (const auto& token_id: std::views::keys(tokensToPoll)) {
                    if (!markedTokensCache.contains(token_id)) {
                        markedTokensCache.insert(token_id);
                        tokensToRefreshMark.emplace_back(token_id);
                    }
                }
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefreshPoller:"
                                            << " tokens_to_poll=" << tokensToPollInitialSize
                                            << " valid_access_tokens=" << validAccessTokensSize
                                            << " failed_token_refreshes=" << failedATRefreshesSize
                                            << " tokens_to_refresh_mark_precache=" << tokensToPoll.size()
                                            << " tokens_to_refresh_mark=" << tokensToRefreshMark.size()
                                            << commit;

            if (!tokensToRefreshMark.empty()) {
                db->markTokensForRefresh(tokensToRefreshMark);
            }
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenRefreshPollerService!" << commit;
            break;
        } catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshPollerService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenRefreshPollerService!" << commit;
        }
    }
}

void TokenRefreshPollerService::registerTokenToRefresh(const std::string& token_id)
{
    boost::unique_lock<boost::shared_mutex> lock(mxPoll);

    if (!tokensToPoll.contains(token_id)) {
        tokensToPoll.emplace(token_id, getTimestampSeconds());
    }
}

std::set<Token> TokenRefreshPollerService::getRefreshedTokens()
{
    boost::unique_lock<boost::shared_mutex> lock(mxRefreshed);
    std::set<Token> results;

    std::swap(results, refreshedTokens);
    return results;
}

TokenRefreshPollerService::FailedRefreshMapType
    TokenRefreshPollerService::getFailedRefreshes()
{
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);
    FailedRefreshMapType results;

    std::swap(results, failedRefreshes);
    return results;
}

template<typename T, typename U>
void TokenRefreshPollerService::removeFromContainer(T& container, const U& token_ids) {
    for (const auto& token_id: token_ids) {
        container.erase(token_id);
    }
}

} // end namespace token
} // end namespace fts3
