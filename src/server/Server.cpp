/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
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

#include "Server.h"

#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "services/cleaner/CleanerService.h"
#include "services/transfers/TransfersService.h"
#include "services/transfers/ReuseTransfersService.h"
#include "services/transfers/ForceStartTransfersService.h"
#include "services/transfers/CancelerService.h"
#include "services/heartbeat/HeartBeat.h"
#include "services/transfers/MessageProcessingService.h"
#include "services/transfers/SupervisorService.h"


using namespace fts3::common;
using namespace fts3::config;

namespace fts3 {
namespace server {


Server::Server()
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Server created" << commit;
}


Server::~Server()
{
    try {
        stop();
        wait();
    }
    catch (...) {
        // pass
    }
    services.clear();
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Server destroyed" << commit;
}


void serviceRunnerHelper(const std::shared_ptr<BaseService>& service) {
    (*service)();
}


void Server::addService(const std::shared_ptr<BaseService>& service)
{
    services.emplace_back(service);
    systemThreads.add_thread(new boost::thread(serviceRunnerHelper, services.back()));
}


void Server::start()
{
    validateConfigRestraints({
        {"MessagingConsumeInterval", "MessagingConsumeGraceTime", 3},
        {"CancelCheckInterval", "CancelCheckGraceTime", 3},
        {"SchedulingInterval", "SchedulingGraceTime", 3}
    });

    auto heartBeatService = std::make_shared<HeartBeat>(processName);

    auto cleanerService = std::make_shared<CleanerService>();
    auto messageProcessingService = std::make_shared<MessageProcessingService>();
    auto cancelerService = std::make_shared<CancelerService>();
    auto transfersService = std::make_shared<TransfersService>();
    auto reuseTransfersService = std::make_shared<ReuseTransfersService>();
    auto supervisorService = std::make_shared<SupervisorService>();
    auto forceStartTransfersService = std::make_shared<ForceStartTransfersService>(heartBeatService);

    // Register "critical" services to be watched by the HeartBeat service
    auto messageProcessingGraceTime = ServerConfig::instance().get<int>("MessagingConsumeGraceTime");
    auto cancelerGraceTime = ServerConfig::instance().get<int>("CancelCheckGraceTime");
    auto transfersGraceTime = ServerConfig::instance().get<int>("SchedulingGraceTime");
    auto supervisorGraceTime = ServerConfig::instance().get<int>("SupervisorGraceTime");

    heartBeatService->registerWatchedService(messageProcessingService, messageProcessingGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(cancelerService, cancelerGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(transfersService, transfersGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(reuseTransfersService, transfersGraceTime, [this] { stop(); });
    heartBeatService->registerWatchedService(supervisorService, supervisorGraceTime, [this] { stop(); });

    addService(heartBeatService);
    addService(cleanerService);

    // Give cleaner and heartbeat some time ahead
    if (!ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(7));
    }

    addService(messageProcessingService);

    // Wait for status updates to be processed
    if (!ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(8));
    }

    addService(cancelerService);
    addService(transfersService);
    addService(reuseTransfersService);
    addService(supervisorService);
    addService(forceStartTransfersService);
}


void Server::wait()
{
    systemThreads.join_all();
}


void Server::stop()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Request to stop the server" << commit;
    systemThreads.interrupt_all();
}


void Server::validateConfigRestraints(const std::vector<ConfigConstraintTuple>& constraints)
{
    for (const auto& constraint: constraints) {
        auto interval = ServerConfig::instance().get<int>(std::get<0>(constraint));
        auto graceTime = ServerConfig::instance().get<int>(std::get<1>(constraint));
        auto factor = std::get<2>(constraint);

        if (graceTime < factor * interval) {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Grace time config constraint failed: "
                                            << std::get<1>(constraint) << "(" << graceTime << ")"
                                            << " < " << factor << " * " << std::get<0>(constraint) << "(" << interval << ")."
                                            << " Aborting process!" << commit;
            exit(1);
        }
    }
}

} // end namespace server
} // end namespace fts3
