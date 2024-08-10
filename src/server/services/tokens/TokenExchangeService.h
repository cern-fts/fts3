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

#include "db/generic/Token.h"
#include "server/common/BaseService.h"
#include "services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class TokenExchangeService: public BaseService
{
public:
    TokenExchangeService(HeartBeat *beat);

    virtual void runService();

    /**
     * Used by the TokenExchangeExecutor workers to publish a successful token-exchange
     * token into the results set.
     *
     * @param exchangedToken The exchanged token
     */
    void registerExchangedToken(const ExchangedToken& exchangedToken);

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

    void exchangeTokens();
    void handleFailedTokenExchange();

private:
    /// Protect concurrent access to "exchangedTokens" set
    boost::shared_mutex mxExchanged;

    /// Set of exchanged tokens obtained via token-exchange requests
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<ExchangedToken> exchangedTokens;

    /// Protect concurrent access to "refreshTokens" set
    boost::shared_mutex mxFailed;

    /// Set of <token_id, message> for failed token-exchange attempts.
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<std::pair<std::string, std::string>> failedExchanges;
};

} // end namespace server
} // end namespace fts3
