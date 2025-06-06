/*
 * Copyright (c) CERN 2023
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

#include "TokenExchangeService.h"
#include "tokens/executors/TokenExchangeExecutor.h"

using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;

namespace fts3 {
namespace token {


TokenExchangeService::TokenExchangeService(const std::shared_ptr<HeartBeat>& heartBeat) :
    BaseService("TokenExchangeService"), heartBeat(heartBeat)
{
    execPoolSize = ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("TokenExchangeCheckInterval");
    bulkSize = ServerConfig::instance().get<int>("TokenExchangeBulkSize");
}

void TokenExchangeService::runService()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenExchangeService interval: " << pollInterval.total_seconds() << "s" << commit;
    int exchangedTokensLastRun = 0;

    while (!boost::this_thread::interruption_requested()) {
        updateLastRunTimepoint();

        // Stand-by if the previous run did not max out the token-exchange bulk size
        if (exchangedTokensLastRun < bulkSize) {
            boost::this_thread::sleep(pollInterval);
        }

        // Reset value here in case unexpected error is thrown in the main try/catch
        exchangedTokensLastRun = 0;

        try {
            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-exchange for this instance!" << commit;
                continue;
            }

            // This service intentionally runs only on the first node
            if (heartBeat->isLeadNode(true)) {
                // Refresh tokens must be obtained for ALL access tokens that don't have one
                exchangedTokensLastRun = exchangeTokens();
                handleFailedTokenExchange();

                // Move the file state from "TOKEN_PREP" to its supposed initial state (stored in "t_token_file_state_initial")
                // Note: The token-exchange can have the following impact on transfers:
                //   - additional wait time, as transfers will start in "TOKEN_PREP" instead of their initial state
                //   - additional chance to fail in the token-exchange step
                db->updateTokenPrepFiles();
            }
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenExchangeService!" << commit;
            break;
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenExchangeService!" << commit;
        }
    }
}

int TokenExchangeService::exchangeTokens()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    ThreadPool<TokenExchangeExecutor> execPool(execPoolSize);

    try {
        auto providers = db->getTokenProviders();
        auto tokens = db->getAccessTokensWithoutRefresh(bulkSize);

        if (tokens.empty()) {
            return 0;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Retrieved " << tokens.size() << " tokens for token-exchange" << commit;

        for (auto& token: tokens) {
            if (boost::this_thread::interruption_requested()) {
                execPool.interrupt();
                return -1;
            }

            auto* exec = new TokenExchangeExecutor(token, providers[token.issuer], *this);
            execPool.start(exec);
        }

        // Wait for all the workers to finish
        execPool.join();

        {
            boost::unique_lock<boost::shared_mutex> lock(mxExchanged);

            for (const auto& it: exchangedTokens) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Storing exchanged token: "
                                                << "token_id=" << it.tokenId << " "
                                                << "expiration_time=" << it.expiry << " "
                                                << "access_token=" << it.accessTokenToString() << " "
                                                << "refresh_token=" << it.refreshTokenToString()
                                                << commit;
            }

            if (!exchangedTokens.empty()) {
                db->storeExchangedTokens(exchangedTokens);
                exchangedTokens.clear();
            }
        }

        return static_cast<int>(tokens.size());
    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Interruption requested in TokenExchangeService:exchangeTokens" << commit;
        execPool.interrupt();
        execPool.join();
        throw;
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService:exchangeTokens: " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenExchangeService:exchangeTokens! " << commit;
    }

    return -1;
}

void TokenExchangeService::handleFailedTokenExchange()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);

    for (const auto& it: failedExchanges) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Handling failed token exchange: "
                                        << "token_id=" << it.first << " "
                                        << "message=\"" << it.second << "\""
                                        << commit;
    }

    if (!failedExchanges.empty()) {
        db->markFailedTokenExchange(failedExchanges);
        db->failTransfersWithFailedTokenExchange(failedExchanges);
        failedExchanges.clear();
    }
}

void TokenExchangeService::registerExchangedToken(const ExchangedToken& exchangedToken)
{
    boost::unique_lock<boost::shared_mutex> lock(mxExchanged);
    exchangedTokens.emplace(exchangedToken);
}

void TokenExchangeService::registerFailedTokenExchange(const std::string& token_id, const std::string& message)
{
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);
    failedExchanges.emplace(token_id, message);
}

} // end namespace token
} // end namespace fts3
