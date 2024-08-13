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

#include "CleanerService.h"

#include <boost/filesystem.hpp>
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "msg-bus/consumer.h"

namespace fs = boost::filesystem;
using fts3::config::ServerConfig;
using fts3::common::commit;

namespace fts3 {
namespace server {


void CleanerService::removeOldFiles(const std::string& path)
{
    fs::recursive_directory_iterator end;

    for (fs::recursive_directory_iterator dir(path); dir != end; ++dir)
    {
        if (!fs::is_directory(*dir))
        {
            std::time_t t = fs::last_write_time(*dir);
            std::time_t now = std::time(NULL);

            double x = difftime(now, t);
            //clean files 15 days old
            if (x > 1296000)
            {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << " Deleting file " << *dir
                << " because it was created " << std::ctime( &t )
                << fts3::common::commit;
                boost::filesystem::remove(*dir);
            }
        }
    }
}


void CleanerService::runService()
{
    int counter = 0;
    auto msgDir = ServerConfig::instance().get<std::string>("MessagingDirectory");
    int purgeMsgDirs = ServerConfig::instance().get<int>("PurgeMessagingDirectoryInterval");
    int checkSanityState = ServerConfig::instance().get<int>("CheckSanityStateInterval");
    int multihopSanitySate = ServerConfig::instance().get<int>("MultihopSanityStateInterval");

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CleanerService interval: 1s" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CleanerService(PurgeMessagingDirectory) interval: " << purgeMsgDirs << "s" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CleanerService(CheckSanityState) interval: " << checkSanityState << "s" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "CleanerService(MultihopSanityState) interval: " << multihopSanitySate << "s" << commit;

    while (!boost::this_thread::interruption_requested())
    {
        ++counter;

        try {
            // Every hour
            if (checkSanityState >0 && counter % checkSanityState == 0) {
                db::DBSingleton::instance().getDBObjectInstance()->checkSanityState();
            }

            // Every 10 minutes
            if (purgeMsgDirs > 0 && counter % purgeMsgDirs == 0) {
                Consumer consumer(msgDir);
                consumer.purgeAll();
            }

            //Every 10 minutes (default)
            if (multihopSanitySate >0 && counter % multihopSanitySate == 0) {
                db::DBSingleton::instance().getDBObjectInstance()->multihopSanitySate();
            }
        } catch(std::exception& e) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files: " << e.what() << commit;
        } catch(...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Cannot delete old files" << commit;
        }

        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }
}

} // end namespace server
} // end namespace fts3
