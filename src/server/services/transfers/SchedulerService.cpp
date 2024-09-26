/*
 * Copyright (c) CERN 2022
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

#include "SchedulerService.h"

#include "common/Logger.h"
#include "common/ThreadPool.h"
#include "common/DaemonTools.h"

#include "config/ServerConfig.h"
#include "cred/DelegCred.h"

#include "db/generic/SingleDbInstance.h"
#include "db/generic/TransferFile.h"

#include "SchedulerService.h"
#include "FileTransferExecutor.h"

using namespace fts3::config;
using namespace fts3::common;

namespace fts3 {
namespace server {

SchedulerService::SchedulerService(const std::shared_ptr<HeartBeat>& heartBeat):
    BaseService("SchedulerService"), heartBeat(heartBeat)
{
    logDir = ServerConfig::instance().get<std::string>("TransferLogDirectory");
    msgDir = ServerConfig::instance().get<std::string>("MessagingDirectory");
    execPoolSize = ServerConfig::instance().get<int>("InternalThreadPool");
    ftsHostName = ServerConfig::instance().get<std::string>("Alias");
    infosys = ServerConfig::instance().get<std::string>("Infosys");

    monitoringMessages = ServerConfig::instance().get<bool>("MonitoringMessaging");
    schedulingInterval = config::ServerConfig::instance().get<boost::posix_time::time_duration>("SchedulingInterval");
}

void SchedulerService::runService() {
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "SchedulerService interval: " << schedulingInterval.total_seconds() << "s" << commit;

    while (!boost::this_thread::interruption_requested()) {
        boost::this_thread::sleep(schedulingInterval);

        try {
            // This service intentionally does not follow the drain mechanism
            if (heartBeat->isLeadNode(true)) {
                db::DBSingleton::instance().getDBObjectInstance()->dummyScheduler();
            }
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in SchedulerService!" << commit;
            break;
        } catch (std::exception &e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in SchedulerService: " << e.what() << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unknown exception in SchedulerService!" << commit;
        }
    }
}
}
}
