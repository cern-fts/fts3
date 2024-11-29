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

#include <boost/thread.hpp>

#include "db/generic/Token.h"
#include "server/common/BaseService.h"

namespace fts3 {
namespace token {

/**
 * The TokenRefreshPoller service is continuously given a set of token IDs
 * to refresh from the TokenRefreshListener service.
 *
 * The poller service continuously polls the database for the given list of ATs
 * that need refreshing. ATs with an acceptable remaining lifetime (e.g.: > 5min)
 * are identified and made available to the TokenRefreshListener service.
 *
 * The tokens that eventually need refreshing are then marked in the database
 * via a special flag, to be picked up and refreshed by the TokenRefresh service.
 * A local cache is kept to prevent duplicates.
 *
 * The next cycle will continue the polling until eventually the ATs are refreshed.
 */
class TokenRefreshPollerService: public fts3::server::BaseService
{
public:
    /// Map for failed token-refreshes: <token_id, (message, timestamp)>
    using FailedRefreshMapType = std::map <std::string, std::pair<std::string, int64_t>>;

    TokenRefreshPollerService();
    virtual ~TokenRefreshPollerService() = default;

    virtual void runService();
    /**
    * Used by the TokenRefreshListener to send tokens that need refreshing
    *
    * @param token_id the token ID to refresh
    */
    void registerTokenToRefresh(const std::string& token_id);

    /**
    * Used by the TokenRefreshListener to get refreshed tokens
    * to send back to the clients
    *
    * @note this operation will empty the refreshed tokens container
    * @return refreshed tokens set
    */
    std::set<Token> getRefreshedTokens();

    /**
     * Used by the TokenRefreshListener to get the set of failed refreshes
     * to send back to the clients
     *
     * @note this operation will empty the failed refreshes container
     * @return failed refreshes map structure
     */
    FailedRefreshMapType getFailedRefreshes();
private:
    /// Helper function to remove an iterable container of token-ids from the target container
    template <class T, class U>
    void removeFromContainer(T& container, const U& token_ids);

    /// Protect concurrent access to "tokensToRefresh" container
    boost::shared_mutex mxPoll;

    /// Map of token-refresh requests <token_id, timestamp> to poll
    std::map<std::string, int64_t> tokensToPoll;

    /// Protect concurrent access to "refreshedTokens" container
    boost::shared_mutex mxRefreshed;

    /// Set of refreshed tokens to send back to the clients
    std::set<Token> refreshedTokens;

    /// Protect concurrent access to "failedRefreshes" container
    boost::shared_mutex mxFailed;

    /// Map of failed token-refreshes <token_id, (message, timestamp)>
    /// to send back to the clients
    FailedRefreshMapType failedRefreshes;

    /// Cache for the tokens already marked for refreshing
    std::set<std::string> markedTokensCache;
};

} // end namespace token
} // end namespace fts3
