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

#include "MultihopTransfersService.h"

#include "common/Logger.h"
#include "server/DrainMode.h"

extern bool stopThreads;

using namespace fts3::common;


namespace fts3 {
namespace server {

extern time_t retrieveRecords;


void MultihopTransfersService::operator () ()
{
    static bool drainMode = false;

    while (!stopThreads)
    {
        retrieveRecords = time(0);

        try
        {
            if (DrainMode::instance())
            {
                if (!drainMode)
                {
                    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Set to drain mode, no more transfers for this instance!" << commit;
                    drainMode = true;
                    sleep(15);
                    continue;
                }
            }
            else
            {
                drainMode = false;
            }

            executeUrlcopy();
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Exception in process_service_multihop_handler " << e.what() << commit;
            sleep(2);
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception in process_service_multihop_handler!" << commit;
            sleep(2);
        }
        sleep(2);
    }
}


void MultihopTransfersService::executeUrlcopy()
{
    std::map<std::string, std::queue<std::pair<std::string, std::list<TransferFile> > > > voQueues;
    DBSingleton::instance().getDBObjectInstance()->getMultihopJobs(voQueues);

    bool empty = false;

    while (!empty)
    {
        empty = true;
        for (auto vo_it = voQueues.begin(); vo_it != voQueues.end(); ++vo_it)
        {
            std::queue<std::pair<std::string, std::list<TransferFile> > > & vo_jobs = vo_it->second;
            if (!vo_jobs.empty())
            {
                empty = false; //< if we are here there are still some data
                std::pair<std::string, std::list<TransferFile> > const job = vo_jobs.front();
                vo_jobs.pop();
                ReuseTransfersService::startUrlCopy(job.first, job.second);
            }
        }
    }
}

} // end namespace server
} // end namespace fts3
