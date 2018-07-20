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
#include "services/transfers/CancelerService.h"
#include "services/heartbeat/HeartBeat.h"
#include "services/optimizer/OptimizerService.h"
#include "services/transfers/MessageProcessingService.h"
#include "services/transfers/SupervisorService.h"


namespace fts3 {
namespace server {


Server::Server()
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Server created" << fts3::common::commit;
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
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "Server destroyed" << fts3::common::commit;
}


void serviceRunnerHelper(std::shared_ptr<BaseService> service)
{
    (*service)();
}


void Server::addService(BaseService *service)
{
    services.emplace_back(service);
    systemThreads.add_thread(new boost::thread(serviceRunnerHelper, services.back()));
}


void Server::start()
{
    auto heartBeatService = new HeartBeat;
    addService(new CleanerService);
    addService(new MessageProcessingService);
    addService(heartBeatService);

    // Give cleaner and heartbeat some time ahead
    if (!config::ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(8));
    }

    addService(new CancelerService);

    // Wait for status updates to be processed
    if (!config::ServerConfig::instance().get<bool> ("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(12));
    }

    addService(new OptimizerService(heartBeatService));
    addService(new TransfersService);
    addService(new ReuseTransfersService);
    addService(new SupervisorService);
}


void Server::wait()
{
    systemThreads.join_all();
}


void Server::stop()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Request to stop the server" << fts3::common::commit;
    systemThreads.interrupt_all();
}

} // end namespace server
} // end namespace fts3
