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
#include "server/DrainMode.h"

#include "TokenExchangeService.h"
#include "TokenExchangeExecutor.h"

#include <cstdlib>

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace server {

extern time_t tokenExchangeRecords;


TokenExchangeService::TokenExchangeService(HeartBeat *beat) : BaseService("TokenExchangeService"), beat(beat)
{
    execPoolSize = config::ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("TokenExchangeCheckInterval");
}

void TokenExchangeService::getRefreshTokens() {
    auto db = db::DBSingleton::instance().getDBObjectInstance();
    ThreadPool<TokenExchangeExecutor> execPool(execPoolSize);

    try {
        auto providers = db->getTokenProviders();

        time_t start = time(nullptr);
        auto tokens = db->getAccessTokensWithoutRefresh();
        time_t end = time(nullptr);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DBtime=\"TokenExchangeService\" "
                                        << "func=\"getRefreshTokens\" "
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

        boost::unique_lock<boost::shared_mutex> lock(mx);

        for (const auto& it: refresh_tokens) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Printing time: "
                                            << "token_id=" << it.first << " "
                                            << "refresh_token=" << it.second
                                            << commit;
        }
        refresh_tokens.clear();

    } catch (const boost::thread_interrupted &) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Interruption requested in TokenExchangeService:getRefreshTokens" << commit;
        execPool.interrupt();
        execPool.join();
    } catch (std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService:getRefreshTokens " << e.what() << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService! " << commit;
    }
}

void TokenExchangeService::runService() {
    // Initialize random seed
    srand(time(nullptr));

    while (!boost::this_thread::interruption_requested()) {
        tokenExchangeRecords = time(nullptr);

        try {
            boost::this_thread::sleep(pollInterval);

            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-exchange for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            // This service intentionally runs only on the first node
            if (beat->isLeadNode(true)) {
                getRefreshTokens();
            }
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenExchangeService!" << commit;
        }
    }
}

void TokenExchangeService::registerRefreshToken(const std::string& token_id, const std::string& refreshToken)
{
    boost::unique_lock<boost::shared_mutex> lock(mx);
    refresh_tokens.emplace(token_id, refreshToken);
}

} // end namespace server
} // end namespace fts3
