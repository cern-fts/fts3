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

#include <davix.hpp>
#include <json/json.h>
#include <boost/any.hpp>
#include <boost/thread.hpp>

#include "IAMWorkflowError.h"
#include "db/generic/Token.h"
#include "db/generic/TokenProvider.h"

namespace fts3 {
namespace token {

/**
 * TokenHTTPExecutor is the base class for HTTP token interactions
 * with the TokenProvider. Common utility functions are added
 * to this layer for reusability.
 */
class TokenHttpExecutor
{
protected:

    TokenHttpExecutor(const Token& token, const TokenProvider& tokenProvider)
        : token(token), tokenProvider(tokenProvider) {}

    virtual ~TokenHttpExecutor() = default;

    /**
     * The main operation, to be implemented by each inheriting variant.
     */
    virtual void run(boost::any &) = 0;

    /**
     * Perform the token HTTP request.
     *
     * @return string containing TokenProvider response
     */
    std::string performTokenHttpRequest();

    /**
     * Find the token endpoint.
     *
     * @return string containing token endpoint URL
     */
    std::string getTokenEndpoint();

    /**
     * Generate the contents of the basic authorization header.
     *
     * @return string containing authorization
     */
    std::string getAuthorizationHeader() const;

    /**
     * Generate the payload to be sent to the authorization server during the token workflow.
     *
     * @return string containing token response data
     */
    virtual std::string getPayloadData() const = 0;

    /**
     * Extract the "error" and "error_description" fields from the server response.
     * This message will be parsed for sensitive values, such as user access token
     * or FTS client ID, which will be redacted. The final message will become
     * the transfer failure reason.
     *
     * @param e the IAM Exchange error containing the server response
     * @param tokenId the token ID, used in logs when server response parsing fails
     * @param secrets map containing <secret, replacement> values
     * @return Final message that will be set as the transfer failure reason
     */
    static std::string extractErrorDescription(const IAMWorkflowError& e,
                                               const std::string& tokenId,
                                               const std::map<std::string, std::string>& secrets);

    /**
     * Execute a Davix HTTP request.
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
     * @param strict throw a Json LogicError if key not found
     * @return string value associated with key
     */
    static std::string parseJson(const std::string& msg, const::std::string& key, bool strict = true);

    /**
     * Check that a Davix Uri is valid
     *
     * @throws DavixException on invalid Uri
     */
    static void validateUri(const Davix::Uri& uri);

    /// Token object
    Token token;
    /// Token provider object
    TokenProvider tokenProvider;

    /// Protect the cached token endpoints map
    static boost::shared_mutex mxTokenEndpoints;

    /// Map of <issuer --> ("token_endpoint", "expire_at")>
    /// Used to cache the ".well-known/openid-configuration" response
    using tokenEndpointMap_t = std::map<std::string, std::pair<std::string, int64_t>>;
    static tokenEndpointMap_t tokenEndpointMap;

private:
    /// Davix context
    Davix::Context context;
};

} // end namespace token
} // end namespace fts3
