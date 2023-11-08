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

#include "common/Logger.h"
#include "common/Exceptions.h"
#include "TokenExchangeExecutor.h"

#include <cryptopp/base64.h>

using namespace fts3::common;

namespace fts3 {
namespace server {

void TokenExchangeExecutor::run([[maybe_unused]] boost::any & ctx)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting token-exchange: "
                                    << "token_id=" << token.tokenId << " "
                                    << "access_token=" << token.accessTokenToString() << " "
                                    << "issuer=" << token.issuer
                                    << commit;

    try {
        auto refresh_token = performTokenExchange();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtained refresh token: "
                                        << "token_id=" << token.tokenId << " "
                                        << "refresh_token=" << refresh_token
                                        << commit;

        tokenExchangeService.registerRefreshToken(token.tokenId, refresh_token);
    } catch (const std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refresh token: "
                                       << "token_id=" << token.tokenId << " "
                                       << e.what() << commit;

        tokenExchangeService.registerFailedTokenExchange(token.tokenId, std::string(e.what()));
    }
}

std::string TokenExchangeExecutor::performTokenExchange()
{
    // GET token exchange endpoint URL
    std::string token_endpoint = getTokenEndpoint();
    Davix::Uri uri(token_endpoint);
    validateUri(uri);

    // Build the POST request
    Davix::DavixError* err = nullptr;
    Davix::PostRequest req(context, uri, &err);

    // Set request parameters
    Davix::RequestParams params;
    params.addHeader("Authorization", getAuthorizationHeader());
    params.addHeader("Content-Type", "application/x-www-form-urlencoded");
    req.setParameters(params);
    req.setRequestBody(getExchangeData());

    // Execute the request
    std::string exchange_result = executeHttpRequest(req);

    // Extract "refresh_token" field from the JSON response
    return parseJson(exchange_result, "refresh_token");
}

std::string TokenExchangeExecutor::getTokenEndpoint()
{
    std::string endpoint = token.issuer + "/.well-known/openid-configuration";

    Davix::Uri uri(endpoint);
    validateUri(uri);

    // Build the GET Request
    Davix::DavixError* err = nullptr;
    Davix::GetRequest req(context, uri, &err);

    // Execute the request
    std::string response = executeHttpRequest(req);

    // Extract "token_endpoint" field from the JSON response
    return parseJson(response, "token_endpoint");
}

std::string TokenExchangeExecutor::getAuthorizationHeader() const
{
    std::string auth_data = tokenProvider.clientId + ":" + tokenProvider.clientSecret;
    std::string encoded_data;
    // Base64 encode the authorization information
    const bool noNewLineInBase64Output = false;
    CryptoPP::StringSource ss1(auth_data, true,
                               new CryptoPP::Base64Encoder(
                                       new CryptoPP::StringSink(encoded_data), noNewLineInBase64Output)
                                       );
    return "Basic " + encoded_data;
}

std::string TokenExchangeExecutor::getExchangeData() const
{
    std::stringstream ss;
    ss << "grant_type=urn:ietf:params:oauth:grant-type:token-exchange"
          "&requested_token_type=urn:ietf:params:oauth:token-type:refresh_token"
          "&subject_token_type=urn:ietf:params:oauth:token-type:access_token"
          "&subject_token=" << token.accessToken <<
          "&scope=" << token.scope;

    // Add optional audience to the token exchange request
    if (!token.audience.empty()) {
        ss << "&audience=" << token.audience;
    }

    return ss.str();
}

} // end namespace server
} // end namespace fts3
