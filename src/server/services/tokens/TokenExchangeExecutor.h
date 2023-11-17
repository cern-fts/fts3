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
#include <davix.hpp>
#include <json/json.h>

#include "TokenExchangeService.h"
#include "IAMExchangeError.h"
#include "db/generic/Token.h"
#include "db/generic/TokenProvider.h"

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

    TokenExchangeExecutor(const Token& token, const TokenProvider& tokenProvider,
                          TokenExchangeService& tokenExchangeService)
        : token(token), tokenProvider(tokenProvider),
          tokenExchangeService(tokenExchangeService), context() {}
    virtual ~TokenExchangeExecutor() = default;

    /**
     * Perform token-exchange for the provided access token
     * in order to obtain a refresh token.
     *
     * @return If successful, the refresh token string is uploaded
     *         to the TokenExchange services
     * @throws UserError in case refresh token could not be retrieved
     */
    virtual void run(boost::any &);

private:

    /**
     * Perform token exchange workflow.
     *
     * @return string containing refresh token
     */
    std::string performTokenExchange();

    /**
     * Finds the token exchange endpoint.
     *
     * @return string containing token exchange URL
     */
    std::string getTokenEndpoint();

    /**
     * Generates the contents of the basic authorization header.
     *
     * @return string containing authorization
     */
    std::string getAuthorizationHeader() const;

    /**
     * Generates the payload to be sent to the authorization server during the token exchange workflow.
     *
     * @return string containing token-exchange data
     */
    std::string getExchangeData() const;

    /**
     * Extract the "error" and "error_description" fields from the server response.
     * This message will be parsed for sensitive values, such as user access token
     * or FTS client ID, which will be redacted. The final message will become
     * the transfer failure reason.
     *
     * @param e the IAM Exchange error containing the server response
     * @return Final message that will be set as the transfer failure reason
     */
    std::string extractErrorDescription(const IAMExchangeError& e);

    /**
     * Executes a Davix HTTP request.
     *
     * @param request reference to a Davix HTTP request object
     * @return string reply form the server
     */
    static std::string executeHttpRequest(Davix::HttpRequest& request);

    /**
     * Parse and find the value of a given key in a JSON message.
     *
     * @param msg a message in JSON format
     * @param key the key to find in the JSON
     * @return string value associated with key
     */
    static std::string parseJson(const std::string& msg, const::std::string& key)
    {
        // Parse the JSON
        Json::Value obj;
        std::stringstream(msg) >> obj;

        std::string res = obj.get(key, "").asString();

        if (res.empty()) {
            std::stringstream error;
            error << "Response JSON did not contain " << key << " key";
            Json::throwLogicError(error.str());
        }

        return res;
    }

    /**
     * Checks that a Davix Uri is valid
     *
     * @throws DavixException on invalid Uri
     */
    static void validateUri(const Davix::Uri& uri)
    {
        Davix::DavixError* err = nullptr;
        Davix::uriCheckError(uri, &err);
        // Throws Davix exception if error is not empty
        Davix::checkDavixError(&err);
    }

    /// Token object
    Token token;
    /// TokenProvider object
    TokenProvider tokenProvider;
    /// Reference to TokenExchange service
    TokenExchangeService& tokenExchangeService;
    /// Davix context
    Davix::Context context;
};

} // end namespace server
} // end namespace fts3
