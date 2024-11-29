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
#include "msg-bus/events.h"
#include "config/ServerConfig.h"
#include "TokenRefreshListenerService.h"

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace token {


TokenRefreshListenerService::TokenRefreshListenerService(const std::shared_ptr<TokenRefreshPollerService>& poller) :
    BaseService("TokenRefreshListenerService"), tokenRefreshPoller(poller),
    zmqContext(1), zmqTokenRouter(zmqContext, zmq::socket_type::router)
{
    auto messagingDirectory = ServerConfig::instance().get<std::string>("MessagingDirectory");
    auto address = "ipc://" + messagingDirectory + "/url_copy-token-refresh.ipc";
    zmqTokenRouter.bind(address);
}

void TokenRefreshListenerService::runService()
{
    while (!boost::this_thread::interruption_requested()) {
        zmq::message_t identity;
        zmq::message_t delimiter;
        zmq::message_t message;

        try {
            updateLastRunTimepoint();
            boost::this_thread::sleep(boost::posix_time::seconds(1));

            while (zmqTokenRouter.recv(identity, zmq::recv_flags::dontwait)) {
                static_cast<void>(zmqTokenRouter.recv(delimiter, zmq::recv_flags::none));
                static_cast<void>(zmqTokenRouter.recv(message, zmq::recv_flags::none));
                fts3::events::TokenRefreshRequest request;

                if (!request.ParseFromArray(message.data(), static_cast<int>(message.size()))) {
                    continue;
                }

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefresh request received:"
                                                << " token_id=" << request.token_id()
                                                << " job_id=" << request.job_id() << " file_id=" << request.file_id()
                                                << " [" << request.process_id() << "@" << request.hostname() << "]"
                                                << commit;

                registerClientRequest(request, std::move(identity));

                // Send the TokenId to the TokenRefreshPoller service
                tokenRefreshPoller->registerTokenToRefresh(request.token_id());
            }

            // Retrieve refreshed tokens from the TokenRefreshPoller service
            auto refreshedTokens = tokenRefreshPoller->getRefreshedTokens();
            dispatchAccessTokens(refreshedTokens);

            // Retrieve refresh failures from the TokenRefreshPoller service
            auto refreshFailures = tokenRefreshPoller->getFailedRefreshes();
            dispatchRefreshFailures(refreshFailures);
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenRefreshListenerService!" << commit;
            break;
        } catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshListenerService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenRefreshListenerService!" << commit;
        }
    }
}

void TokenRefreshListenerService::registerClientRequest(
    const fts3::events::TokenRefreshRequest& request, zmq::message_t&& identity)
{
    /**
     * Ideally, this function would use a set, but this will create many problems
     * with the fact zmq::message_t cannot be "const" and copy-constructed, and the std::set strong "const" requirement.
     * Over all, it's easier to use a list and ensure no duplicates are inserted
     */
    bool duplicate = false;
    auto token_id = request.token_id();


    if (!routingMap.contains(token_id)) {
        routingMap[token_id] = std::list<ZMQ_client>();
    }

    for (const auto& it: routingMap[token_id]) {
        if (it.identifier == identity) {
            duplicate = true;
            break;
        }
    }

    if (!duplicate) {
        routingMap[token_id].emplace_back(ZMQ_client{
                std::move(identity),
                request.job_id(), request.file_id(),
                request.process_id(), request.hostname()
        });
    }
}

void TokenRefreshListenerService::dispatchAccessTokens(const std::set<Token>& tokens)
{
    for (const auto& token: tokens) {
        const auto& map_it = routingMap.find(token.tokenId);

        if (map_it != routingMap.end()) {
            fts3::events::TokenRefreshResponse response;
            response.set_response_type(fts3::events::TokenRefreshResponse::TYPE_ACCESS_TOKEN);
            response.set_token_id(token.tokenId);
            response.set_access_token(token.accessToken);
            response.set_expiry_timestamp(token.expiry);
            auto serialized = response.SerializeAsString();

            for (auto& it: map_it->second) {
                zmqTokenRouter.send(it.identifier, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{0}, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{serialized}, zmq::send_flags::none);

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefresh response sent:"
                                << " token_id=" << response.token_id()
                                << " expiration_time=" << response.expiry_timestamp()
                                << " job_id=" << it.job_id << " file_id=" << it.file_id
                                << " [" << it.process_id << "@" << it.hostname << "]"
                                << commit;
            }

            routingMap.erase(map_it);
        }
    }
}

void TokenRefreshListenerService::dispatchRefreshFailures(
    const TokenRefreshPollerService::FailedRefreshMapType& refreshFailures)
{
    for (const auto& [token_id, msgPair]: refreshFailures) {
        const auto& map_it = routingMap.find(token_id);

        if (map_it != routingMap.end()) {
            fts3::events::TokenRefreshResponse response;
            response.set_response_type(fts3::events::TokenRefreshResponse::TYPE_REFRESH_FAILURE);
            response.set_token_id(token_id);
            response.set_refresh_message(msgPair.first);
            response.set_refresh_timestamp(msgPair.second);
            auto serialized = response.SerializeAsString();

            for (auto& it: map_it->second) {
                zmqTokenRouter.send(it.identifier, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{0}, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{serialized}, zmq::send_flags::none);

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefresh failed-refresh sent:"
                                << " token_id=" << response.token_id()
                                << " refresh_message=\"" << response.refresh_message() << "\""
                                << " refresh_timestamp=" << response.refresh_timestamp()
                                << " job_id=" << it.job_id << " file_id=" << it.file_id
                                << " [" << it.process_id << "@" << it.hostname << "]"
                                << commit;
            }
        }

        routingMap.erase(map_it);
    }

}

} // end namespace token
} // end namespace fts3
