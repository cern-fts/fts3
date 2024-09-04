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

#include <utility>

#include "HeartBeat.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "server/common/DrainMode.h"
#include "server/Server.h"

using namespace fts3::common;
using namespace fts3::config;

namespace fts3 {
namespace server {


HeartBeat::HeartBeat(std::string processName):
    BaseService("HeartBeat"),
    index(0), count(0), start(0), end(0),
    processName(std::move(processName)) {}

void HeartBeat::runService()
{
    using TDuration = boost::posix_time::time_duration;
    auto heartBeatInterval = ServerConfig::instance().get<TDuration>("HeartBeatInterval");
    auto heartBeatGraceInterval = ServerConfig::instance().get<TDuration>("HeartBeatGraceInterval");

    if (heartBeatInterval >= heartBeatGraceInterval) {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT)
            << "HeartBeatInterval >= HeartBeatGraceInterval. Can not work like this" << commit;
        _exit(1);
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Using heartbeat interval: " << heartBeatInterval.total_seconds() << "s" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Using heartbeat grace interval: " << heartBeatGraceInterval.total_seconds() << "s" << commit;


    while (!boost::this_thread::interruption_requested()) {
        try {
            if (DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more heartbeat for this instance!" << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            criticalServiceExpired();

            DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                &index, &count, &start, &end, processName);

            FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Systole: host " << index << " out of " << count
                                             << " [" << start << ':' << end << ']' << commit;
            // Sleep on successful update (on exception, repeat after 1s)
            boost::this_thread::sleep(heartBeatInterval);
        } catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested in HeartBeat!" << commit;
            break;
        } catch (const std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Heartbeat failed: " << e.what() << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        } catch (...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Heartbeat failed " << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
    }
}

void HeartBeat::criticalServiceExpired()
{
    for (const auto& [service, data]: watchedServices) {
        auto serviceName = service->getServiceName();
        auto last = service->getLastRunTimepoint();
        auto graceTime = data.graceTime;
        auto fun_gracefulAbort = data.fun_gracefulAbort;

        auto now = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - last).count();

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Service \"" << serviceName << "\"" 
                                         << " time_since_last_run=" << diff << "s" << commit;

        if (diff >= graceTime) {
            // Aborting the daemon
            FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Service \"" << serviceName << "\" inactive for too long!"
                                            << " (" << diff << "s >= " << graceTime << "s)"
                                            << " Aborting daemon!" << commit;

            // Offer grace time for all threads to finish gracefully
            fun_gracefulAbort();
            boost::this_thread::sleep(boost::posix_time::seconds(10));
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Exiting process" << commit;
            exit(1);
        }
    }
}

bool HeartBeat::isLeadNode(bool bypassDraining) const
{   
    if (DrainMode::instance() && !bypassDraining) {
        return false;
    } 

    return index == 0;
}

} // end namespace server
} // end namespace fts3
