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
#include "IAMExchangeError.h"

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
        auto exchanged_token = performTokenExchange();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtained exchanged token: "
                                        << "token_id=" << token.tokenId << " "
                                        << "access_token=" << exchanged_token.accessTokenToString() << " "
                                        << "refresh_token=" << exchanged_token.refreshToken
                                        << commit;

        tokenExchangeService.registerExchangedToken(exchanged_token);
    } catch (const IAMExchangeError& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refresh token: "
                                       << "token_id=" << token.tokenId << " "
                                       << "response: '" << e.what() << "'" << commit;

        auto message = extractErrorDescription(e);
        tokenExchangeService.registerFailedTokenExchange(token.tokenId, message);
    } catch (const std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refresh token: "
                                       << "token_id=" << token.tokenId << " "
                                       << "error: '" << e.what() << "'" << commit;

        tokenExchangeService.registerFailedTokenExchange(token.tokenId, e.what());
    }
}

ExchangedToken TokenExchangeExecutor::performTokenExchange()
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

    // Construct exchanged access token from the JSON response
    return {
            token.tokenId,
            parseJson(exchange_result, "access_token", false),
            parseJson(exchange_result, "refresh_token"),
            token.accessToken
    };
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

std::string TokenExchangeExecutor::executeHttpRequest(Davix::HttpRequest& request) {
    Davix::DavixError* req_error = nullptr;
    Davix::DavixError* response_error = nullptr;
    std::vector<char> buffer(DAVIX_BLOCK_SIZE);
    std::ostringstream response;

    // Perform a Davix request and manually read the response content
    // sent by the server
    //
    // Explanation: The standard Davix "executeRequest()"
    // won't read the body content in case of a non-successful reply.
    // To circumvent this, we emulate the "executeRequest()" steps
    // and read the response ourselves, in a read loop

    request.beginRequest(&req_error);

    while (request.readBlock(&buffer[0], DAVIX_BLOCK_SIZE, &response_error) > 0) {
        Davix::checkDavixError(&response_error);
        response << buffer.data();
        buffer.clear();
    }

    request.endRequest(nullptr);

    if (request.getRequestCode() != 200) {
        // Throw an IAM Exception containing the response body
        // The response message will later be parsed for relevant info.
        // In case no relevant info is found, the initial request error is returned
        throw IAMExchangeError(request.getRequestCode(), response.str(), req_error);
    }

    return response.str();
}

std::string TokenExchangeExecutor::extractErrorDescription(const IAMExchangeError& e) {
    // Attempt to extract meaningful message from the Token Provider response.
    // We do this by searching for the "error" and "error_description" JSON fields.
    // (IAM returns a JSON response describing the error)

    try {
        auto type = parseJson(e.what(), "error");
        auto description = parseJson(e.what(), "error_description");

        auto _replace = [&description](const std::string& search, const std::string& replace) {
            auto pos = description.find(search);

            if (pos != std::string::npos) {
                description.replace(pos, search.size(), replace);
            }
        };

        // Replace secrets, such as token value or FTS client ID
        // The parsed output will be the transfer failure message
        _replace(token.accessToken, token.accessTokenToString());
        _replace(tokenProvider.clientId, "<FTS client ID>");
        return "[" + type + "]: " + description;
    } catch (const std::exception& tmp) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Failed to extract \"error_description\" field from server response: "
                                         << "token_id=" << token.tokenId << " exception=\"" << tmp.what() << "\" "
                                         << "response: '" << e.what() << "'" << commit;
        return e.davix_error();
    }
}

} // end namespace server
} // end namespace fts3
