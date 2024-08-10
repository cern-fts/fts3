/*
 * Copyright (c) CERN 2013-2019
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

#include <string>
#include <set>
#include <unordered_map>
#include <vector>

#include "common/Exceptions.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"
#include "server/common/DrainMode.h"

#include "FetchCancelArchiving.h"
#include "../task/ArchivingPollTask.h"


void FetchCancelArchiving::fetch()
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchCancelArchiving starting" << commit;
    while (!boost::this_thread::interruption_requested()) {
        try
        {
            boost::this_thread::sleep(boost::posix_time::seconds(60));

            //if we drain a host, no need to check if url_copy are reporting being alive
            if (fts3::server::DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                continue;
            }

            std::set<std::pair<std::string, std::string>> urls;
            db::DBSingleton::instance().getDBObjectInstance()->getArchivingFilesForCanceling(urls);

            if (!urls.empty())
            {
                ArchivingPollTask::cancel(urls);
            }
        }
        catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchCancelArchiving Error " << e.what() << commit;
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchCancelArchiving interruption requested" << commit;
            break;
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FetchCancelArchiving Unknown error" << commit;
        }
    }
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FetchCancelArchiving exiting" << commit;
}
