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
#include "server/services/heartbeat/HeartBeat.h"
#include "optimizer/services/OptimizerService.h"
#include "OptimizerServer.h"

using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server;

namespace fts3 {
namespace optimizer {

void serviceRunnerHelper(const std::shared_ptr<BaseService>& service) {
    (*service)();
}

OptimizerServer::OptimizerServer()
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Optimizer server created" << commit;
}

OptimizerServer::~OptimizerServer()
{
    try {
        stop();
        wait();
    } catch (const std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Exception when stopping Optimizer server!"
                                           << " (exception=" << ex.what() << ")" << commit;
    } catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Unknown exception when stopping Optimizer server!" << commit;
    }

    services.clear();
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Optimizer server destroyed" << commit;
}

void OptimizerServer::addService(const std::shared_ptr<BaseService>& service)
{
    services.emplace_back(service);
    systemThreads.add_thread(new boost::thread(serviceRunnerHelper, services.back()));
}

void OptimizerServer::start()
{
    auto heartBeatService = std::make_shared<HeartBeat>();
    addService(heartBeatService);

    // Give heartbeat some time ahead
    if (!ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(8));
    }

    addService(std::make_shared<OptimizerService>(heartBeatService));
}

void OptimizerServer::wait()
{
    systemThreads.join_all();
}

void OptimizerServer::stop()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Request to stop the Optimizer server" << commit;
    systemThreads.interrupt_all();
}

} // end namespace optimizer
} // end namespace fts3