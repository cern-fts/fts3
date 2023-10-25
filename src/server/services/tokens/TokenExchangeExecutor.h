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

#include <boost/thread.hpp>
#include <boost/any.hpp>

#include "db/generic/Token.h"

namespace fts3 {
namespace server {

/**
 * TokenExchangeExecutor is the worker class that performs the token-exchange workflow
 * for the given Token in order to obtain a refresh token.
 * This work is done in a separate thread.
 *
 * The Tokens are added to a thread-safe queue, which is processed by the worker.
 * Calling 'join()' will wait for the thread to finish processing Tokens.
  */
class TokenExchangeExecutor
{
public:

    TokenExchangeExecutor(const Token& token) : token(token) {}
    virtual ~TokenExchangeExecutor() = default;

    /**
     * Performs token-exchange for the provided access token
     * in order to obtain a refresh token
     *
     * @return string containing the refresh token
     * @throws error in case refresh token could not be retrieved
     */
    virtual void run(boost::any &);

private:
    Token token;
};

} // end namespace server
} // end namespace fts3
