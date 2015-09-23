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
#include "db/generic/SingleDbInstance.h"
#include "server/DrainMode.h"


using namespace fts3::common;
extern bool stopThreads;


namespace fts3 {
namespace server {

time_t retrieveRecords = time(0);
time_t updateRecords = time(0);
time_t stallRecords = time(0);


void HeartBeat::operator () ()
{
    while (!stopThreads)
    {
        try
        {
            //if we drain a host, we need to let the other hosts know about it, hand-over all files to the rest
            if (DrainMode::instance())
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)
                        << "Set to drain mode, no more transfers for this instance!"
                        << commit;
                sleep(15);
                continue;
            }

            if (criticalThreadExpired(retrieveRecords, updateRecords, stallRecords))
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                        << "One of the critical threads looks stalled"
                        << commit;
                // Note: Would be nice to get the pstack output in the log
                orderedShutdown();
            }
            else
            {
                unsigned index = 0, count = 0, start = 0, end = 0;
                std::string serviceName = "fts_server";

                try
                {

                    db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                            &index, &count, &start, &end, serviceName);
                    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host "
                            << index << " out of " << count << " [" << start
                            << ':' << end << ']' << commit;
                }
                catch (const std::exception& e)
                {
                    try
                    {
                        sleep(5);
                        db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                                &index, &count, &start, &end, serviceName);
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host "
                                << index << " out of " << count << " ["
                                << start << ':' << end << ']' << commit;
                    }
                    catch (const std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR)
                                << "Hearbeat failed: " << e.what()
                                << commit;

                    }
                    catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed "
                                << commit;
                    }
                }
                catch (...)
                {
                    try
                    {
                        sleep(5);
                        db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                                &index, &count, &start, &end, serviceName);
                        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host "
                                << index << " out of " << count << " ["
                                << start << ':' << end << ']' << commit;
                    }
                    catch (const std::exception& e)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR)
                                << "Hearbeat failed: " << e.what()
                                << commit;

                    }
                    catch (...)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed " << commit;
                    }
                }
            }
            sleep(60);
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed: " << e.what() << commit;
            sleep(60);
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Hearbeat failed " << commit;
            sleep(60);
        }
    }
}


bool HeartBeat::criticalThreadExpired(time_t retrieveRecords, time_t updateRecords ,time_t stallRecords)
{
    double diffTime = 0.0;

    diffTime = std::difftime(std::time(NULL), retrieveRecords);
    if (diffTime > 7200)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Wall time passed retrieve records: " << diffTime
                << " secs " << commit;
        return true;
    }

    diffTime = std::difftime(std::time(NULL), updateRecords);
    if (diffTime > 7200)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Wall time passed update records: " << diffTime
                << " secs " << commit;
        return true;
    }

    diffTime = std::difftime(std::time(NULL), stallRecords);
    if (diffTime > 10000)
    {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Wall time passed stallRecords and cancelation thread exited: "
                << diffTime << " secs " << commit;
        return true;
    }

    return false;
}

void HeartBeat::orderedShutdown()
{
    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Stopping other threads..." << commit;
    // Give other threads a chance to finish gracefully
    stopThreads = true;
    sleep(30);

    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exiting" << commit;
    _exit(1);
}

} // end namespace server
} // end namespace fts3
