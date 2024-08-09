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
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>

#include "common/Exceptions.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"


namespace fs = boost::filesystem;
using namespace fts3::config;
using namespace fts3::common;
using namespace db;


static void initializeDbBackend()
{
    std::string dbType = ServerConfig::instance().get<std::string > ("DbType");
    std::string dbUserName = ServerConfig::instance().get<std::string>("DbUserName");
    std::string dbPassword = ServerConfig::instance().get<std::string>("DbPassword");
    std::string dbConnectString = ServerConfig::instance().get<std::string>("DbConnectString");

    const std::string experimentalPostgresSupport =
        ServerConfig::instance().get<std::string> ("ExperimentalPostgresSupport");
    if (dbType == "postgresql" && experimentalPostgresSupport != "true") {
        throw std::runtime_error(
            "Failed to initialize database: "
            "DbType cannot be set to postgresql if ExperimentalPostgresSupport is not set to true"
        );
    }

    db::DBSingleton::instance().getDBObjectInstance()->init(dbType, dbUserName, dbPassword, dbConnectString, 1);
}


int main(int argc, char **argv)
{
    try {
        const char *configfile = "/etc/fts3/fts3config";

        if (!fs::exists(configfile)) {
            std::cerr << "fts3 server config file doesn't exist" << std::endl;
            exit(1);
        }

        ServerConfig::instance().read(argc, argv);

        std::string logPath = ServerConfig::instance().get<std::string>("ServerLogDirectory");
        if (logPath.length() > 0) {
            logPath += "/fts3server.log";
            if (theLogger().redirect(logPath, logPath) < 0) {
                throw SystemError("Database cleaner failed to open the log file");
            }
        }
        theLogger().setLogLevel(Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel")));

        initializeDbBackend();

        long bulkSize = ServerConfig::instance().get<long>("CleanBulkSize");
        int cleanInterval = ServerConfig::instance().get<int>("CleanInterval");

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Backup starting with bulk size of " << bulkSize
            << " and an interval of " << cleanInterval << " days" << commit;

        long nJobs = 0, nFiles = 0, nDeletions = 0;
        auto start = boost::chrono::steady_clock::now();
        db::DBSingleton::instance().getDBObjectInstance()->backup(cleanInterval, bulkSize, &nJobs, &nFiles, &nDeletions);
        auto end = boost::chrono::steady_clock::now();

        auto duration = boost::chrono::duration_cast<boost::chrono::seconds>(end - start);

        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Backup ending: "
        << nJobs << " jobs, "
        << nFiles << " files and "
        << nDeletions << " deletions affected after "
        << duration.count() << " seconds"
        << commit;

    }
    catch (const std::exception &e) {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Backup fatal error, exiting... " << e.what() << commit;
        return EXIT_FAILURE;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(CRIT) << "Backup fatal error (unknown origin), exiting..." << commit;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
