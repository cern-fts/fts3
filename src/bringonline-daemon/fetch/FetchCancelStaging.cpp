/*
 * FetchCancelStaging.cpp
 *
 *  Created on: 3 Jul 2014
 *      Author: simonm
 */

#include "FetchCancelStaging.h"

#include "task/BringOnlineTask.h"

#include "server/DrainMode.h"

#include "db/generic/SingleDbInstance.h"

#include "common/logger.h"
#include "common/error.h"

#include <boost/tuple/tuple.hpp>

#include <vector>
#include <string>
#include <unordered_map>
#include <set>

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
