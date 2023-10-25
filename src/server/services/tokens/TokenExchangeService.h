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

#include "services/BaseService.h"
#include "services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace server {

class TokenExchangeService: public BaseService
{
public:
    TokenExchangeService(HeartBeat *beat);
    virtual void runService();

protected:
    int execPoolSize;
    boost::posix_time::time_duration pollInterval;

    HeartBeat *beat;
    void getRefreshTokens();
};

} // end namespace server
} // end namespace fts3
