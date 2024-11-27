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

#pragma once

#include "db/generic/Token.h"
#include "server/common/BaseService.h"
#include "server/services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace token {

class TokenRefreshService: public fts3::server::BaseService
{
public:
    TokenRefreshService(const std::shared_ptr<fts3::server::HeartBeat>& heartBeat);
    virtual ~TokenRefreshService() = default;

    virtual void runService();

    /**
     * Used by the TokenRefreshExecutor workers to publish a successful token-refresh
     * token into the results set.
     *
     * @param refreshedToken The refreshed token
     */
    void registerRefreshedToken(const RefreshedToken& refreshedToken);

    /**
     * Used by the TokenRefreshExecutor workers to publish a failed token-refresh attempt.
     *
     * @param token_id The token ID
     * @param message The error message encountered
     */
    void registerFailedTokenRefresh(const std::string& token_id, const std::string& message);

protected:
    int bulkSize;
    int execPoolSize;
    boost::posix_time::time_duration pollInterval;
    const std::shared_ptr<fts3::server::HeartBeat> heartBeat;

    int refreshTokens();
    void handleFailedTokenRefresh();

private:
    /// Protect concurrent access to "refreshedTokens" set
    boost::shared_mutex mxRefreshed;

    /// Set of exchanged tokens obtained via token-exchange requests
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<RefreshedToken> refreshedTokens;

    /// Protect concurrent access to "failedRefreshes" set
    boost::shared_mutex mxFailed;

    /// Set of <token_id, message> for failed token-refresh attempts.
    /// The set is populated by the worker threads
    /// and will be collected at the end of a cycle
    std::set<std::pair<std::string, std::string>> failedRefreshes;
};

} // end namespace token
} // end namespace fts3
