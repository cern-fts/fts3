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
#include "common/TimeUtils.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "TokenRefreshPollerService.h"

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace token {


TokenRefreshPollerService::TokenRefreshPollerService() : BaseService("TokenRefreshPollerService"),
    cacheCleanupTimestamp(0)
{}

void TokenRefreshPollerService::runService()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();

    while (!boost::this_thread::interruption_requested()) {
        try {
            updateLastRunTimepoint();
            boost::this_thread::sleep(boost::posix_time::seconds(15));

            std::list<std::string> tokensToRefreshMark;
            size_t tokensToPollInitialSize;
            size_t validAccessTokensSize;

            {
                boost::unique_lock<boost::shared_mutex> lockQuery(mxPoll);

                if (tokensToPoll.empty()) {
                   continue;
                }

                tokensToPollInitialSize = tokensToPoll.size();

                // Look-up the tokens-to-refresh-poll set in the database
                auto validAccessTokens = db->getValidAccessTokens(tokensToPoll);
                validAccessTokensSize = validAccessTokens.size();

                // Make the refreshed tokens available to the TokenRefreshListener service
                {
                    boost::unique_lock<boost::shared_mutex> lockResults(mxRefreshed);
                    refreshedTokens.insert(validAccessTokens.begin(), validAccessTokens.end());
                }

                // Remove valid access tokens from the tokens-to-refresh-poll set
                for (const auto& token: validAccessTokens) {
                     auto it = tokensToPoll.find(token.tokenId);

                     if (it != tokensToPoll.end()) {
                         tokensToPoll.erase(it);
                     }
                }

                // Mark the remaining tokens-to-refresh-poll in the database for refreshing
                // Note: we use an in-memory cache to prevent excessive database marking
                for (const auto& token_id: tokensToPoll) {
					auto it = markedTokensCache.find(token_id);

                    if ((it == markedTokensCache.end()) ||
                        (getTimestampSeconds() >= it->second)) {
                        tokensToRefreshMark.emplace_back(token_id);
                        markedTokensCache.insert_or_assign(token_id, getTimestampSeconds(60));
                    }
                }
            }

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefreshPoller:"
                                            << " tokens_to_poll=" << tokensToPollInitialSize
                                            << " valid_access_tokens=" << validAccessTokensSize
                                            << " tokens_to_refresh_precache=" << tokensToPoll.size()
                                            << " tokens_to_refresh=" << tokensToRefreshMark.size()
                                            << commit;

            if (!tokensToRefreshMark.empty()) {
                db->markTokensForRefresh(tokensToRefreshMark);
            }

            cacheCleanup();
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
    tokensToPoll.emplace(token_id);
}

std::set<Token> TokenRefreshPollerService::getRefreshedTokens()
{
    boost::unique_lock<boost::shared_mutex> lock(mxRefreshed);
    std::set<Token> results;

    std::swap(results, refreshedTokens);
    return results;
}

void TokenRefreshPollerService::cacheCleanup()
{
    auto now = getTimestampSeconds();

    if (now >= cacheCleanupTimestamp) {
        cacheCleanupTimestamp = now + 60;
        auto threshold = 600;

        for (auto it = markedTokensCache.begin(); it != markedTokensCache.end(); ) {
            if (now - it->second >= threshold) {
                it = markedTokensCache.erase(it);
            } else {
                ++it;
            }
        }
    }
}

} // end namespace token
} // end namespace fts3
