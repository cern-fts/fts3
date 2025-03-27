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

#include <boost/any.hpp>

#include "TokenHttpExecutor.h"
#include "db/generic/Token.h"
#include "db/generic/TokenProvider.h"
#include "tokens/services/TokenRefreshService.h"

namespace fts3 {
namespace token {

/**
 * TokenRefreshExecutor is the worker class that performs the token-refresh workflow
 * for the given Token in order to obtain a refreshed access token.
 * This work is done in a separate thread.
 *
 * The RefreshedTokens are added to a thread-safe queue, which is processed by the worker.
 * Calling 'join()' will wait for the thread to finish processing Tokens.
  */
class TokenRefreshExecutor final : public TokenHttpExecutor
{
public:

    TokenRefreshExecutor(const Token& token, const TokenProvider& tokenProvider,
                         TokenRefreshService& tokenRefreshService)
        : TokenHttpExecutor("TokenRefresh", token, tokenProvider),
          tokenRefreshService(tokenRefreshService) {}

    virtual ~TokenRefreshExecutor() = default;

    /**
     * Perform token-refresh workflow for the provided access token
     *
     * @return If successful, the refreshed access token string is uploaded
     *         to the TokenRefresh services
     * @throws UserError in case token-refresh could not be performed
     */
    void run(boost::any &) override;

protected:
    /**
     * Generates the token-refresh payload data
     *
     * @return string containing token-refresh data
     */
    std::string getPayloadData() const override;

private:
    /**
     * Perform the token-refresh workflow.
     *
     * @return refreshed access token object
     */
    RefreshedToken performTokenRefresh();

    /// Reference to TokenRefresh service
    TokenRefreshService& tokenRefreshService;
};

} // end namespace token
} // end namespace fts3
