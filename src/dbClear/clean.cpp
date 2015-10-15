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

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <boost/filesystem.hpp>

#include "../config/ServerConfig.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"
#include "profiler/Profiler.h"


namespace fs = boost::filesystem;
using namespace fts3::config; 
using namespace fts3::common;
using namespace db;

/* -------------------------------------------------------------------------- */
void fts3_initialize_db_backend()
{
    std::string dbUserName = ServerConfig::instance().get<std::string>("DbUserName");
    std::string dbPassword = ServerConfig::instance().get<std::string>("DbPassword");
    std::string dbConnectString = ServerConfig::instance().get<std::string>("DbConnectString");

    try
        {
            db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 1);
        }
    catch(UserError& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << e.what() << commit;
            exit(1);
        }
    catch(...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something is going on with the database, check username/password/connstring" << commit;
            exit(1);
        }
}


int main(int argc, char** argv)
{

    try
        {
            const char *configfile = "/etc/fts3/fts3config";

            if (!fs::exists(configfile))
                {
                    std::cerr << "fts3 server config file doesn't exist"  << std::endl;
                    exit(1);
                }

            ServerConfig::instance().read(argc, argv);

            std::string logDir = ServerConfig::instance().get<std::string > ("ServerLogDirectory");
            if (logDir.length() > 0)
                {
                    logDir += "/fts3server.log";
                    if (theLogger().redirect(logDir, logDir) < 0)
                        {
                            std::cerr << "fts3 server failed to open log file, errno is:" << strerror(errno) << std::endl;
                            exit(1);
                        }
                }


            fts3_initialize_db_backend();

            long bulkSize = ServerConfig::instance().get<long>("CleanBulkSize");

            FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Backup starting with bulk size of " << bulkSize << commit;
            long nJobs = 0, nFiles = 0;

            db::DBSingleton::instance().getDBObjectInstance()->backup(bulkSize, &nJobs, &nFiles);

            FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Backup ending: "
                                           << nJobs << " jobs and "
                                           << nFiles << " files affected"
                                           << commit;

            // If profiling is configured, dump the timing
            db::DBSingleton::instance().getDBObjectInstance()->storeProfiling(&fts3::ProfilingSubsystem::instance());

        }
    catch (const std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Backup fatal error, exiting... " << e.what() << commit;
            return EXIT_FAILURE;
        }
    catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Backup fatal error (unknown origin), exiting..." << commit;
            return EXIT_FAILURE;
        }

    return EXIT_SUCCESS;
}
