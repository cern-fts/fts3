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

#pragma once

#include "services/BaseService.h"
#include "services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class TokenExchangeService: public BaseService
{
public:
    TokenExchangeService(HeartBeat *beat);

    virtual void runService();

    /**
     * Used by the TokenExchangeExecutor workers to publish a successful
     * (tokenId, refreshToken) pair into the results set.
     *
     * @param token_id The token ID
     * @param refreshToken The refresh token value
     */
    void registerRefreshToken(const std::string& token_id, const std::string& refreshToken);

    /**
     * Used by the TokenExchangeExecutor workers to publish a failed token-exchange attempt.
     *
     * @param token_id The token ID
     * @param message The error message encountered
     */
    void registerFailedTokenExchange(const std::string& token_id, const std::string& message);

protected:
    int execPoolSize;
    boost::posix_time::time_duration pollInterval;
    HeartBeat *beat;

    void getRefreshTokens();
    void handleFailedTokenExchange();

private:
    /// Protect concurrent access to "refreshTokens" set
    boost::shared_mutex mxRefresh;

    /// Set of <token_id, refresh_token> for refresh tokens
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<std::pair<std::string, std::string>> refreshTokens;

    /// Protect concurrent access to "refreshTokens" set
    boost::shared_mutex mxFailed;

    /// Set of <token_id, message> for failed token-exchange attempts.
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<std::pair<std::string, std::string>> failedExchanges;

    bool impatientDebugger;
};

} // end namespace server
} // end namespace fts3
