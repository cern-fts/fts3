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

#include <iostream>
#include <cstdio>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include "config/serverconfig.h"
#include "db/generic/SingleDbInstance.h"
#include "profiler/Profiler.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../common/Exceptions.h"
#include "../common/Logger.h"

using namespace fts3::config; 
using namespace fts3::common;
using namespace db;

/* -------------------------------------------------------------------------- */
void fts3_initialize_db_backend()
{
    std::string dbUserName = theServerConfig().get<std::string>("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string>("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string>("DbConnectString");

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

static int fexists(const char *filename)
{
    struct stat buffer;
    if (::stat(filename, &buffer) == 0)
        return 0;
    return -1;
}



int main(int argc, char** argv)
{

    try
        {
            const char *configfile = "/etc/fts3/fts3config";

            if (fexists(configfile) != 0)
                {
                    std::cerr << "fts3 server config file doesn't exist"  << std::endl;
                    exit(1);
                }

            theServerConfig().read(argc, argv);

            std::string logDir = theServerConfig().get<std::string > ("ServerLogDirectory");
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

            std::string cleanRecordsHost = theServerConfig().get<std::string>("CleanRecordsHost");


            FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Backup starting" << commit;
            long nJobs = 0, nFiles = 0;

            db::DBSingleton::instance().getDBObjectInstance()->backup(&nJobs, &nFiles);

            FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Backup ending: "
                                           << nJobs << " jobs and "
                                           << nFiles << " files affected"
                                           << commit;

            // If profiling is configured, dump the timing
            db::DBSingleton::instance().getDBObjectInstance()->storeProfiling(&fts3::ProfilingSubsystem::instance());

        }
    catch (BaseException& e)
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
