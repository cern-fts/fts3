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
#include "TokenRefreshPollerService.h"

namespace fts3 {
namespace token {

/**
 * This class listens to incoming ZMQ clients, which bring token-refresh requests.
 * The token_ids are stored and passed along to the TokenRefreshPoller service
 * which handles the interaction with the database.
 *
 * Tokens that need refreshing are passed to the TokenRefreshPoller service
 * via a set of token IDs, which all need refreshing.
 *
 * Periodically, this service queries the TokenRefreshPoller for the
 * list of refreshed tokens. The refreshed tokens are then dispatched
 * back to the ZMQ clients using a ZMQ routing map.
 */
class TokenRefreshListenerService: public fts3::server::BaseService
{
public:
    TokenRefreshListenerService(const std::shared_ptr<TokenRefreshPollerService>& poller);
    virtual ~TokenRefreshListenerService() = default;

    virtual void runService();

protected:
    const std::shared_ptr<TokenRefreshPollerService> tokenRefreshPoller;

    /// ZMQ setup
    zmq::context_t zmqContext;
    zmq::socket_t zmqTokenRouter;

private:
    /// Register ZMQ client in the message routing map
    void registerClientRequest(const std::string& token_id, zmq::message_t&& identity);

    /// Dispatch refresh tokens to ZMQ clients
    void dispatchClientResponses(const std::set<Token>& tokens);

    /// Message routing map <tokenId --> [list of ZMQ identifiers]>
    std::map<std::string, std::list<zmq::message_t>> routingMap;
};

} // end namespace token
} // end namespace fts3
