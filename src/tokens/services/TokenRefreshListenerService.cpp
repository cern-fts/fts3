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
#include "common/DaemonTools.h"
#include "msg-bus/events.h"
#include "config/ServerConfig.h"
#include "server/common/DrainMode.h"
#include "TokenRefreshListenerService.h"

using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;

namespace fts3 {
namespace token {


TokenRefreshListenerService::TokenRefreshListenerService() : BaseService("TokenRefreshListenerService"),
    zmqContext(1), zmqTokenRouter(zmqContext, zmq::socket_type::router)
{
    auto messagingDirectory = ServerConfig::instance().get<std::string>("MessagingDirectory");
    auto address = "ipc://" + messagingDirectory + "/url_copy-token-refresh.ipc";
    zmqTokenRouter.bind(address);
}

void TokenRefreshListenerService::runService() {

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

                registerClientRequest(request.token_id(), std::move(identity));
            }

            // Send the token_id to the TokenRefreshPoller service
            // tokenRefreshPoller.add(request.token_id());

            // Retrieve refreshed tokens from the TokenRefreshPoller service
            // tokenRefreshPoller.get();

            std::set<std::string> token_ids;
            for (const auto& it: routingMap) {
                token_ids.insert(it.first);
            }

            dispatchClientResponses(token_ids);
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in TokenRefreshService!" << commit;
            break;
        } catch (std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in TokenRefreshService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in TokenRefreshService!" << commit;
        }
    }
}

void TokenRefreshListenerService::registerClientRequest(const std::string& token_id, zmq::message_t&& identity)
{
    /**
     * Ideally, this function would use a set, but this will create many problems
     * with the fact zmq::message_t cannot be "const" and copy-constructed, and the std::set strong "const" requirement.
     * Over all, it's easier to use a list and ensure no duplicates are inserted
     */
    bool duplicate = false;

    if (!routingMap.contains(token_id)) {
        routingMap[token_id] = std::list<zmq::message_t>();
    }

    for (const auto& it: routingMap[token_id]) {
        if (it == identity) {
            duplicate = true;
            break;
        }
    }

    if (!duplicate) {
        routingMap[token_id].emplace_back(std::move(identity));
    }
}

void TokenRefreshListenerService::dispatchClientResponses(const std::set<std::string>& token_ids)
{
    for (const auto& token_id: token_ids) {
        const auto& map_it = routingMap.find(token_id);

        if (map_it != routingMap.end()) {
            for (auto& it: map_it->second) {
                std::string reply_message = "Refreshed token for: " + map_it->first;
                zmqTokenRouter.send(it, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{0}, zmq::send_flags::sndmore);
                zmqTokenRouter.send(zmq::message_t{reply_message}, zmq::send_flags::none);
            }

            routingMap.erase(map_it);
        }
    }
}

} // end namespace token
} // end namespace fts3
