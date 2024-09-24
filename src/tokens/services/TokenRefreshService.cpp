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
#include "common/ThreadPool.h"

#include "config/ServerConfig.h"
#include "server/common/DrainMode.h"

#include "TokenRefreshService.h"

#include <cstdlib>

using namespace fts3::config;
using namespace fts3::common;
using namespace fts3::server;

namespace fts3 {
namespace token {


TokenRefreshService::TokenRefreshService() :
    BaseService("TokenRefreshService")
{
    execPoolSize = ServerConfig::instance().get<int>("InternalThreadPool");
    pollInterval = ServerConfig::instance().get<boost::posix_time::time_duration>("TokenRefreshInterval");
}

void TokenRefreshService::runService() {
    auto db = db ::DBSingleton::instance().getDBObjectInstance();

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "TokenRefreshService interval: " << pollInterval.total_seconds() << "s" << commit;

    while (!boost::this_thread::interruption_requested()) {
        updateLastRunTimepoint();
        boost::this_thread::sleep(pollInterval);

        try {
            if (DrainMode::instance()) {
              FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more token-refreshing for this instance!" << commit;
              boost::this_thread::sleep(boost::posix_time::seconds(15));
              continue;
            }


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

} // end namespace token
} // end namespace fts3
