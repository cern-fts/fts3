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
#include "services/BaseService.h"
#include "services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class TokenRefreshService: public BaseService
{
public:
    TokenRefreshService(HeartBeat *beat);

    virtual void runService();

    /**
     * Used by the TokenRefreshExecutor workers to publish a successful
     * refreshed access token into the results set.
     *
     * @param refreshedToken The refreshed access token
     */
    void registerRefreshedAccessToken(const RefreshedToken& refreshToken);

    /**
     * Used by the TokenRefreshExecutor workers to publish a failed token-refresh attempt.
     *
     * @param token_id The token ID
     * @param message The error message encountered
     */
    void registerFailedTokenRefresh(const std::string& token_id, const std::string& message);

protected:
    int execPoolSize;
    boost::posix_time::time_duration pollInterval;
    HeartBeat *beat;

    void refreshAccessTokens();
    void handleFailedTokenRefresh();

private:
    /// Protect concurrent access to "refreshedTokens" set
    boost::shared_mutex mxRefresh;

    /// Set of <token_id, access_token, refresh_token> for refreshed access tokens
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

} // end namespace server
} // end namespace fts3
