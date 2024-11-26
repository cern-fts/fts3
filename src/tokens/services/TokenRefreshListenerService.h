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

#include <zmq.hpp>

#include "db/generic/Token.h"
#include "server/common/BaseService.h"
#include "server/services/heartbeat/HeartBeat.h"

namespace fts3 {
namespace token {

class TokenRefreshListenerService: public fts3::server::BaseService
{
public:
    TokenRefreshListenerService();
    virtual ~TokenRefreshListenerService() = default;

    virtual void runService();

protected:
    /// ZMQ setup
    zmq::context_t zmqContext;
    zmq::socket_t zmqTokenRouter;

private:
    /// Register ZMQ client in the message routing map
    void registerClientRequest(const std::string& token_id, zmq::message_t&& identity);

    /// Dispatch refresh tokens to ZMQ clients
    void dispatchClientResponses(const std::set<std::string>& token_ids);

    /// Message routing map <tokenId --> [list of ZMQ identifiers]>
    std::map<std::string, std::list<zmq::message_t>> routingMap;
};

} // end namespace token
} // end namespace fts3
