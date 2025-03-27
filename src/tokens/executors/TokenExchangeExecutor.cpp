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
#include "db/generic/DbUtils.h"

#include "IAMWorkflowError.h"
#include "TokenExchangeExecutor.h"

using namespace fts3::common;

namespace fts3 {
namespace token {

void TokenExchangeExecutor::run([[maybe_unused]] boost::any & ctx)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting token-exchange: "
                                    << "token_id=" << token.tokenId << " "
                                    << "expiration_time=" << token.expiry << " "
                                    << "issuer=" << token.issuer << " "
                                    << "access_token=" << token.accessTokenToString()
                                    << commit;

    try {
        auto exchanged_token = performTokenExchange();

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Obtained exchanged token: "
                                        << "token_id=" << token.tokenId << " "
                                        << "expiration_time=" << exchanged_token.expiry << " "
                                        << "access_token=" << exchanged_token.accessTokenToString() << " "
                                        << "refresh_token=" << exchanged_token.refreshTokenToString()
                                        << commit;

        tokenExchangeService.registerExchangedToken(exchanged_token);
    } catch (const IAMWorkflowError& e) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to obtain refresh token: "
                                       << "token_id=" << token.tokenId << " "
                                       << "response: '" << e.what() << "'" << commit;

        auto message = extractErrorDescription(e, token.tokenId, {
                                                   { token.accessToken, token.accessTokenToString() },
                                                   { tokenProvider.clientId, "<FTS client ID>" }
                                               });
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
    auto response = performTokenHttpRequest();

    // Construct exchanged token from the JSON response
    time_t expiry{token.expiry};
    auto expiresIn = parseJson(response, "expires_in", false);

    if (!expiresIn.empty()) {
        expiry = db::getUTC(std::stoi(expiresIn));
    }

    return {
            token.tokenId,
            parseJson(response, "access_token", false),
            parseJson(response, "refresh_token"),
            token.accessToken,
            expiry
    };
}

std::string TokenExchangeExecutor::getPayloadData() const
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


} // end namespace token
} // end namespace fts3
