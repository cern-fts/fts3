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

#include "common/Logger.h"
#include "common/TimeUtils.h"

#include "IAMWorkflowError.h"
#include "TokenHttpExecutor.h"

#include <cryptopp/base64.h>

using namespace fts3::common;

namespace fts3 {
namespace token {

boost::shared_mutex TokenHttpExecutor::mxTokenEndpoints;
TokenHttpExecutor::tokenEndpointMap_t TokenHttpExecutor::tokenEndpointMap;

std::string TokenHttpExecutor::performTokenHttpRequest()
{
    // GET token endpoint URL
    std::string token_endpoint = getTokenEndpoint();
    Davix::Uri uri(token_endpoint);
    validateUri(uri);

    // Build the POST request
    Davix::DavixError* err = nullptr;
    Davix::PostRequest req(context, uri, &err);
    auto payloadData = getPayloadData();

    // Set request parameters
    Davix::RequestParams params;
    params.addHeader("Authorization", getAuthorizationHeader());
    params.addHeader("Content-Type", "application/x-www-form-urlencoded");
    req.setParameters(params);
    req.setRequestBody(payloadData);

    // Execute the request
    FTS3_COMMON_LOGGER_NEWLOG(TOKEN) << "[" << name << "::" << token.tokenId << "]: > " << payloadData << commit;
    auto response = executeHttpRequest(req);
    FTS3_COMMON_LOGGER_NEWLOG(TOKEN) << "[" << name << "::" << token.tokenId << "]: < " << response << commit;

    return response;
}

std::string TokenHttpExecutor::getTokenEndpoint()
{
    // Look into the token endpoint map first
    {
        boost::unique_lock<boost::shared_mutex> lock(mxTokenEndpoints);
        auto it = tokenEndpointMap.find(token.issuer);

        if (it != tokenEndpointMap.end()) {
            if (getTimestampSeconds() < it->second.second) {
                FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Found cached token endpoint: "
                                                 << token.issuer << " --> " << it->second.first
                                                 << " (expire_at=" << it->second.second <<  ")" << commit;
                return it->second.first;
            }
        }
    }

    // Retrieve the "token_endpoint" via the .well-known endpoint
    std::string endpoint = token.issuer + ".well-known/openid-configuration";

    Davix::Uri uri(endpoint);
    validateUri(uri);

    // Build the GET Request
    Davix::DavixError* err = nullptr;
    Davix::GetRequest req(context, uri, &err);

    // Execute the request
    std::string response = executeHttpRequest(req);

    // Extract "token_endpoint" field from the JSON response
    auto tokenEndpoint = parseJson(response, "token_endpoint");

    // Save "token_endpoint" value into token endpoint cache map
    {
        boost::unique_lock<boost::shared_mutex> lock(mxTokenEndpoints);
        auto expireAt = getTimestampSeconds(3600);
        tokenEndpointMap[token.issuer] = { tokenEndpoint, expireAt };

        FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Storing cached token endpoint: "
                                         << token.issuer << " --> " << tokenEndpoint
                                         << " (expire_at=" << expireAt << ")" << commit;
    }

    return tokenEndpoint;
}

std::string TokenHttpExecutor::getAuthorizationHeader() const
{
    constexpr bool noNewLineInBase64Output = false;

    std::string auth_data = tokenProvider.clientId + ":" + tokenProvider.clientSecret;
    std::string encoded_data;

    // Base64 encode the authorization information
    CryptoPP::StringSource ss1(auth_data, true,
                               new CryptoPP::Base64Encoder(
                                       new CryptoPP::StringSink(encoded_data), noNewLineInBase64Output)
                                       );
    return "Basic " + encoded_data;
}

std::string TokenHttpExecutor::executeHttpRequest(Davix::HttpRequest& request)
{
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

    while (true) {
        auto bytesRead = request.readBlock(&buffer[0], DAVIX_BLOCK_SIZE, &response_error);
        if (bytesRead <= 0) {
            break;
        }
        Davix::checkDavixError(&response_error);
        response.write(buffer.data(), bytesRead); // Write the actual data read
    }

    request.endRequest(nullptr);

    if (request.getRequestCode() != 200) {
        // Throw an IAM Exception containing the response body
        // The response message will later be parsed for relevant info.
        // In case no relevant info is found, the initial request error is returned
        throw IAMWorkflowError(request.getRequestCode(), response.str(), req_error);
    }

    return response.str();
}

std::string TokenHttpExecutor::extractErrorDescription(const IAMWorkflowError& e,
                                                       const std::string& tokenId,
                                                       const std::map<std::string, std::string>& secrets)
{
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
        for (const auto& [secret, replacement]: secrets) {
            _replace(secret, replacement);
        }

        return "[" + type + "]: " + description;
    } catch (const std::exception& tmp) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Failed to extract \"error_description\" field from server response: "
                                         << "token_id=" << tokenId << " exception=\"" << tmp.what() << "\" "
                                         << "response: '" << e.what() << "'" << commit;
        return e.davix_error();
    }
}

std::string TokenHttpExecutor::parseJson(const std::string& msg, const::std::string& key, const bool strict)
{
    Json::Value obj;
    std::stringstream(msg) >> obj;
    std::string res = obj.get(key, "").asString();

    if (res.empty() && strict) {
        std::stringstream error;
        error << "Response JSON did not contain " << key << " key";
        Json::throwLogicError(error.str());
    }

    return res;
}

void TokenHttpExecutor::validateUri(const Davix::Uri& uri)
{
    Davix::DavixError* err = nullptr;
    Davix::uriCheckError(uri, &err);
    // Throws Davix exception if error is not empty
    Davix::checkDavixError(&err);
}

} // end namespace token
} // end namespace fts3
