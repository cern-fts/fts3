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

#include <boost/any.hpp>

#include "TokenHttpExecutor.h"
#include "db/generic/Token.h"
#include "db/generic/TokenProvider.h"
#include "tokens/services/TokenExchangeService.h"

namespace fts3 {
namespace token {

/**
 * TokenExchangeExecutor is the worker class that performs the token-exchange workflow
 * for the given Token in order to obtain a refresh token.
 * This work is done in a separate thread.
 *
 * The Tokens are added to a thread-safe queue, which is processed by the worker.
 * Calling 'join()' will wait for the thread to finish processing Tokens.
  */
class TokenExchangeExecutor final : public TokenHttpExecutor
{
public:

    TokenExchangeExecutor(const Token& token, const TokenProvider& tokenProvider,
                          TokenExchangeService& tokenExchangeService)
        : TokenHttpExecutor("TokenExchange", token, tokenProvider),
          tokenExchangeService(tokenExchangeService) {}

    virtual ~TokenExchangeExecutor() = default;

    /**
     * Perform token-exchange for the provided access token
     * in order to obtain a refresh token.
     *
     * @return If successful, the refresh token string is uploaded
     *         to the TokenExchange services
     * @throws UserError in case refresh token could not be retrieved
     */
    void run(boost::any &) override;

protected:
    /**
     * Generates the token-exchange payload data
     *
     * @return string containing token-exchange data
     */
    std::string getPayloadData() const override;

private:
    /**
     * Perform token exchange workflow.
     *
     * @return exchanged access token object
     */
    ExchangedToken performTokenExchange();

    /// Reference to TokenExchange service
    TokenExchangeService& tokenExchangeService;
};

} // end namespace token
} // end namespace fts3
