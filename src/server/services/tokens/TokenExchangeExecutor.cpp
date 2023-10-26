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

#include <cstdlib>

using namespace fts3::common;

namespace fts3 {
namespace server {

void TokenExchangeExecutor::run(boost::any & ctx)
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
        throw UserError(std::string(__func__) + "Failed to obtain refresh token: " + e.what());
    }
}

std::string TokenExchangeExecutor::performTokenExchange()
{
    auto get_rand_char = []() -> char {
        const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };

    std::string refresh_token(128, 0);
    std::generate_n(refresh_token.begin(), 128, get_rand_char);
    return refresh_token;
}

} // end namespace server
} // end namespace fts3
