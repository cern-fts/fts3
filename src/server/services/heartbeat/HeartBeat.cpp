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

#include "HeartBeat.h"

#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "server/DrainMode.h"
#include "server/Server.h"

using namespace fts3::common;
using namespace fts3::config;

namespace fts3 {
namespace server {

time_t retrieveRecords = time(0);
time_t updateRecords = time(0);
time_t stallRecords = time(0);


HeartBeat::HeartBeat(): BaseService("HeartBeat"), index(0), count(0), start(0), end(0)
{
}


void HeartBeat::runService()
{
    int heartBeatInterval, heartBeatGraceInterval;
    try {
        heartBeatInterval = ServerConfig::instance().get<int>("HeartBeatInterval");
        heartBeatGraceInterval = ServerConfig::instance().get<int>("HeartBeatGraceInterval");
        if (heartBeatInterval >= heartBeatGraceInterval) {
            FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                << "HeartBeatInterval >= HeartBeatGraceInterval. Can not work like this" << commit;
            _exit(1);
        }
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Could not get the heartbeat interval" << commit;
        _exit(1);
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Using heartbeat interval " << heartBeatInterval << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Using heartbeat grace interval " << heartBeatGraceInterval << commit;


    while (!boost::this_thread::interruption_requested())
    {
        try
        {
            //if we drain a host, we need to let the other hosts know about it, hand-over all files to the rest
            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                        << "Set to drain mode, no more transfers for this instance!"
                        << commit;
                boost::this_thread::sleep(boost::posix_time::seconds(15));
                continue;
            }

            if (criticalThreadExpired(retrieveRecords, updateRecords, stallRecords))
            {
                FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                        << "One of the critical threads looks stalled"
                        << commit;
                // Note: Would be nice to get the pstack output in the log
                orderedShutdown();
            }

            std::string serviceName = "fts_server";

            db::DBSingleton::instance().getDBObjectInstance()
                ->updateHeartBeat(&index, &count, &start, &end, serviceName);
            FTS3_COMMON_LOGGER_NEWLOG(DEBUG)
                << "Systole: host " << index << " out of " << count
                << " [" << start << ':' << end << ']'
                << commit;

            // It the update was successful, we sleep here
            // If it wasn't, only one second will pass until the next retry
            boost::this_thread::sleep(boost::posix_time::seconds(heartBeatInterval));
        }
        catch (const boost::thread_interrupted&)
        {
            throw;
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed: " << e.what() << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed " << commit;
            boost::this_thread::sleep(boost::posix_time::seconds(1));
        }
    }
}


bool HeartBeat::criticalThreadExpired(time_t retrieveRecords, time_t updateRecords ,time_t stallRecords)
{
    double diffTime = 0.0;

    diffTime = std::difftime(std::time(NULL), retrieveRecords);
    if (diffTime > 7200)
    {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                << "Wall time passed retrieve records: " << diffTime
                << " secs " << commit;
        return true;
    }

    diffTime = std::difftime(std::time(NULL), updateRecords);
    if (diffTime > 7200)
    {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                << "Wall time passed update records: " << diffTime
                << " secs " << commit;
        return true;
    }

    diffTime = std::difftime(std::time(NULL), stallRecords);
    if (diffTime > 10000)
    {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT)
                << "Wall time passed stallRecords and cancelation thread exited: "
                << diffTime << " secs " << commit;
        return true;
    }

    return false;
}

void HeartBeat::orderedShutdown()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Stopping other threads..." << commit;
    // Give other threads a chance to finish gracefully
    Server::instance().stop();
    boost::this_thread::sleep(boost::posix_time::seconds(30));

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Exiting" << commit;
    exit(1);
}

bool HeartBeat::isLeadNode()
{
    return index == 0;
}

} // end namespace server
} // end namespace fts3
