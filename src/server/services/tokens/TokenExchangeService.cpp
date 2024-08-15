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
#include "TokenExchangeExecutor.h"

#include <cstdlib>

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace server {


TokenExchangeService::TokenExchangeService(const std::shared_ptr<HeartBeat>& heartBeat) :
    BaseService("TokenExchangeService"), heartBeat(heartBeat)
{
    execPoolSize = ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("TokenExchangeCheckInterval");
}

void TokenExchangeService::runService() {

    auto db = db::DBSingleton::instance().getDBObjectInstance();

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenExchangeService interval: " << pollInterval.total_seconds() << "s" << commit;

    while (!boost::this_thread::interruption_requested()) {
        updateLastRunTimepoint();
        boost::this_thread::sleep(pollInterval);

        try {
            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-exchange for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            // This service intentionally runs only on the first node
            if (heartBeat->isLeadNode(true)) {
                exchangeTokens();
                handleFailedTokenExchange();
                // The below function does not require any state from the service
                // Refresh tokens must be obtained for ALL access tokens that don't have one.

                // Move the file state from "TOKEN_PREP" to its supposed initial state (stored in "t_token_file_state_initial")
                // Note: The token-exchange can have the following impact on transfers:
                //   - additional wait time, as transfers will start in "TOKEN_PREP" instead of their initial state
                //   - additional chance to fail in the token-exchange step
                db->updateTokenPrepFiles();
            }
        } catch (boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenExchangeService!" << commit;
            break;
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService!" << commit;
        }
    }
}

void TokenExchangeService::exchangeTokens() {
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    ThreadPool<TokenExchangeExecutor> execPool(execPoolSize);

    try {
        auto providers = db->getTokenProviders();

        time_t start = time(nullptr);
        auto tokens = db->getAccessTokensWithoutRefresh();
        time_t end = time(nullptr);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TokenExchangeService\" "
                                        << "func=\"exchangeTokens\" "
                                        << "DBcall=\"getAccessTokensWithoutRefresh\" "
                                        << "time=\"" << end - start << "\""
                                        << commit;

        if (tokens.empty()) {
            return;
        }

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Retrieved " << tokens.size() << " tokens for token-exchange" << commit;

        for (auto& token: tokens) {
            if (boost::this_thread::interruption_requested()) {
                execPool.interrupt();
                return;
            }

            TokenExchangeExecutor *exec =
                    new TokenExchangeExecutor(token, providers[token.issuer], *this);
            execPool.start(exec);
        }

        // Wait for all the workers to finish
        execPool.join();

        {
            boost::unique_lock<boost::shared_mutex> lock(mxExchanged);

            for (const auto& it: exchangedTokens) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Storing exchanged token: "
                                                << "token_id=" << it.tokenId << " "
                                                << "access_token=" << it.accessTokenToString() << " "
                                                << "refresh_token=" << it.refreshToken
                                                << commit;
            }

            if (!exchangedTokens.empty()) {
                db->storeExchangedTokens(exchangedTokens);
                exchangedTokens.clear();
            }
        }
    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TokenExchangeService:exchangeTokens" << commit;
        execPool.interrupt();
        execPool.join();
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService:exchangeTokens " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService! " << commit;
    }
}

void TokenExchangeService::handleFailedTokenExchange()
{
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    boost::unique_lock<boost::shared_mutex> lock(mxFailed);

    for (const auto& it: failedExchanges) {
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Handling failed token exchange: "
                                        << "token_id=" << it.first << " "
                                        << "message=" << it.second
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

} // end namespace server
} // end namespace fts3
