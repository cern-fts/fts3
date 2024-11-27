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
    * @note this operation will empty the refreshed tokens structure
    * @return refreshed tokens set
    */
    std::set<Token> getRefreshedTokens();

private:
    /// Clean the cache every 10 minutes
    void cacheCleanup();

    /// Protect concurrent access to "tokensToRefresh" set
    boost::shared_mutex mxPoll;

    /// Set of token ids to poll for refresh
    std::set<std::string> tokensToPoll;

    /// Protect concurrent access to "refreshedTokens" set
    boost::shared_mutex mxRefreshed;

    /// Set of refreshed tokens to send back to the clients
    std::set<Token> refreshedTokens;

    /// Timestamp for when to perform next tokens cache cleanup
    int64_t cacheCleanupTimestamp;

    /// Cache for the tokens already marked for refreshing
    /// Implemented as map of <token_id, valid_until_timestamp> (
    std::map<std::string, int64_t> markedTokensCache;
};

} // end namespace token
} // end namespace fts3
