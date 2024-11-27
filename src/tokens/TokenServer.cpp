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
#include "config/ServerConfig.h"
#include "server/Server.h"
#include "server/services/heartbeat/HeartBeat.h"
#include "services/TokenExchangeService.h"
#include "services/TokenRefreshPollerService.h"
#include "services/TokenRefreshListenerService.h"
#include "TokenServer.h"

using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server;

namespace fts3 {
namespace token {

void serviceRunnerHelper(const std::shared_ptr<BaseService>& service) {
    (*service)();
}

TokenServer::TokenServer()
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Token server created" << commit;
}

TokenServer::~TokenServer()
{
    try {
        stop();
        wait();
    } catch (const std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Exception when stopping Token server!"
                                           << " (exception=" << ex.what() << ")" << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Unknown exception when stopping Token server!" << commit;
    }

    services.clear();
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Token server destroyed" << commit;
}

void TokenServer::addService(const std::shared_ptr<BaseService>& service)
{
    services.emplace_back(service);
    systemThreads.add_thread(new boost::thread(serviceRunnerHelper, services.back()));
}

void TokenServer::start()
{
    Server::validateConfigRestraints({
        {"TokenExchangeCheckInterval", "TokenExchangeCheckGraceTime", 3},
        {"TokenRefreshInterval", "TokenRefreshGraceTime", 3},
    });

    auto heartBeatService = std::make_shared<HeartBeat>(processName);
    auto tokenExchangeService = std::make_shared<TokenExchangeService>(heartBeatService);
    auto tokenRefreshPollerService = std::make_shared<TokenRefreshPollerService>();
    auto tokenRefreshListenerService = std::make_shared<TokenRefreshListenerService>(tokenRefreshPollerService);

    // Register te TokenExchange service as "critical" to be watched by the HeartBeat service
    auto tokenExchangeGraceTime = ServerConfig::instance().get<int>("TokenExchangeCheckGraceTime");
    auto tokenRefreshGraceTime = ServerConfig::instance().get<int>("TokenRefreshGraceTime");

    heartBeatService->registerWatchedService(tokenExchangeService, tokenExchangeGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(tokenRefreshListenerService, tokenRefreshGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(tokenRefreshPollerService, tokenRefreshGraceTime, [this] { stop(); });

    addService(heartBeatService);

    // Give heartbeat some time ahead
    if (!ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(7));
    }

    addService(tokenExchangeService);
    addService(tokenRefreshPollerService);
    addService(tokenRefreshListenerService);
}

void TokenServer::wait()
{
    systemThreads.join_all();
}

void TokenServer::stop()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Request to stop the Token server" << commit;
    systemThreads.interrupt_all();
}

} // end namespace token
} // end namespace fts3
