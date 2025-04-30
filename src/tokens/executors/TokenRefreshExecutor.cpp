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
#include "db/generic/DbUtils.h"

#include "IAMWorkflowError.h"
#include "TokenRefreshExecutor.h"

using namespace fts3::common;

namespace fts3 {
namespace token {

void TokenRefreshExecutor::run([[maybe_unused]] boost::any & ctx)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting token-refresh: "
                                    << "token_id=" << token.tokenId << " "
                                    << "expiration_time=" << token.expiry << " "
                                    << "issuer=" << token.issuer << " "
                                    << "access_token=" << token.accessTokenToString()
                                    << commit;

    try {
        auto refreshed_token = performTokenRefresh();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtained refreshed access token: "
                                        << "token_id=" << refreshed_token.tokenId << " "
                                        << "expiration_time=" << refreshed_token.expiry << " "
                                        << "access_token=" << refreshed_token.accessTokenToString() << " "
                                        << "refresh_token=" << refreshed_token.refreshTokenToString()
                                        << commit;

        tokenRefreshService.registerRefreshedToken(refreshed_token);
    } catch (const IAMWorkflowError& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to refresh access token: "
                                       << "token_id=" << token.tokenId << " "
                                       << "response: '" << e.what() << "'" << commit;

        auto message = extractErrorDescription(e, token.tokenId, {
                                                    { token.accessToken, token.accessTokenToString() },
                                                    { token.refreshToken, token.refreshTokenToString() },
                                                    { tokenProvider.clientId, "<FTS client ID>" }
                                               });
        tokenRefreshService.registerFailedTokenRefresh(token.tokenId, message);
    } catch (const std::exception& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to refresh access token: "
                                       << "token_id=" << token.tokenId << " "
                                       << "error: '" << e.what() << "'" << commit;

        tokenRefreshService.registerFailedTokenRefresh(token.tokenId, e.what());
    }
}

RefreshedToken TokenRefreshExecutor::performTokenRefresh()
{
    auto response = performTokenHttpRequest();

    // Construct refreshed token from the JSON response
    return {
        token.tokenId,
        parseJson(response, "access_token"),
        parseJson(response, "refresh_token", false),
        token.refreshToken,
        db::getUTC(std::stoi(parseJson(response, "expires_in")))
    };
}

std::string TokenRefreshExecutor::getPayloadData() const
{
    std::stringstream ss;
    ss << "grant_type=refresh_token"
        "&refresh_token=" << token.refreshToken <<
        "&scope=" << token.scope;

    // Add optional audience to the token refresh request
    if (!token.audience.empty()) {
        ss << "&audience=" << token.audience;
    }

    return ss.str();
}

} // end namespace token
} // end namespace fts3
