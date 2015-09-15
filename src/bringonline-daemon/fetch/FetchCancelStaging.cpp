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

#include "FetchCancelStaging.h"

#include "../task/BringOnlineTask.h"

#include "server/DrainMode.h"

#include "db/generic/SingleDbInstance.h"

#include "common/error.h"

#include <boost/tuple/tuple.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include "../../common/Logger.h"

extern bool stopThreads;

void FetchCancelStaging::fetch()
{

    while(!stopThreads)
        {
            try  //this loop must never exit
                {
                    //if we drain a host, no need to check if url_copy are reporting being alive
                    if (DrainMode::getInstance())
                        {
                            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                            boost::this_thread::sleep(boost::posix_time::milliseconds(15000));
                            continue;
                        }

                    std::set<std::pair<std::string, std::string>> urls;
                    db::DBSingleton::instance().getDBObjectInstance()->getStagingFilesForCanceling(urls);

                    if (!urls.empty())
                        {

                            // do the cancellation
                            BringOnlineTask::cancel(urls);
                        }
                    // sleep for 10 seconds
                    boost::this_thread::sleep(boost::posix_time::milliseconds(10000));

                }
            catch (Err& e)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE " << e.what() << commit;
                }
            catch (...)
                {
                    sleep(2);
                    FTS3_COMMON_LOGGER_NEWLOG(ERR) << "BRINGONLINE Fatal error (unknown origin)" << commit;
                }
        }
}
