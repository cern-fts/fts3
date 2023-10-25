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
#include "TokenExchangeExecutor.h"

using namespace fts3::common;

namespace fts3 {
namespace server {

void TokenExchangeExecutor::run(boost::any & ctx)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Starting token-exchange: "
                                    << "access_token=" << token.accessTokentoString() << " "
                                    << "issuer=" << token.getIssuer()
                                    << commit;
}

} // end namespace server
} // end namespace fts3
