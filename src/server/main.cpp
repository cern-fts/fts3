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

/** \file main.cpp FTS3 server entry point. */

#include <signal.h>

#include <boost/filesystem.hpp>
#include <sstream>
#include <common/PidTools.h>

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "common/panic.h"
#include "db/generic/SingleDbInstance.h"

#include "Server.h"


namespace fs = boost::filesystem;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server;


/// Initialize the database backend
static void intializeDatabase()
{
    auto dbType = ServerConfig::instance().get<std::string>("DbType");
    auto dbUserName = ServerConfig::instance().get<std::string>("DbUserName");
    auto dbPassword = ServerConfig::instance().get<std::string>("DbPassword");
    auto dbConnectString = ServerConfig::instance().get<std::string>("DbConnectString");
    int pooledConn = ServerConfig::instance().get<int>("DbThreadsNum");

    auto experimentalPostgresSupport =
        ServerConfig::instance().get<std::string>("ExperimentalPostgresSupport");

    if (dbType == "postgresql" && experimentalPostgresSupport != "true") {
        throw std::runtime_error(
            "Failed to initialize database: "
            "DbType cannot be set to postgresql if ExperimentalPostgresSupport is not set to true"
        );
    }

    db::DBSingleton::instance().getDBObjectInstance()->init(
        dbType, dbUserName, dbPassword, dbConnectString, pooledConn);
}

/// Called by the signal handler
static void shutdownCallback(int signum, void*)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Future signals will be ignored!" << commit;

    Server::instance().stop();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;

    // Some require traceback
    switch (signum)
    {
        case SIGABRT: case SIGSEGV: case SIGILL:
        case SIGFPE: case SIGBUS: case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(CRIT)<< "Stack trace: \n"
                << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size)
                << commit;
            break;
        default:
            break;
    }
}

/// Main body of the FTS3 server
/// The configuration should have been loaded already
static void doServer(void)
{
    setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

    bool isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");
    std::string logPath = ServerConfig::instance().get<std::string>("ServerLogDirectory");

    if (isDaemon && logPath.length() > 0) {
        logPath += "/fts3server.log";
        if (theLogger().redirect(logPath, logPath) != 0) {
            std::ostringstream msg;
            msg << "fts3 server failed to open log file " << logPath << " error is:" << strerror(errno);
            throw SystemError(msg.str());
        }
    }
    theLogger().setLogLevel(Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel")));
    theLogger().setProfiling(ServerConfig::instance().get<bool>("Profiling"));

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Starting server... (process_id=" << getpid() << ")" << commit;

    intializeDatabase();
    Server::instance().start();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Server started" << commit;

    Server::instance().wait();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Server stopped" << commit;

    exit(0);
}


static std::string requiredToString(int mode)
{
    char strMode[] = "---";

    if (mode & R_OK)
        strMode[0] = 'r';
    if (mode & W_OK)
        strMode[1] = 'w';
    if (mode & X_OK)
        strMode[2] = 'x';

    return strMode;
}


/// Validate the file or directory path exists, has the right type
/// and permissions
/// @param mode    as for access (R_OK, W_OK, ...)
/// @param type    file type
static void checkPath(const std::string& path, int mode, fs::file_type type)
{
    if (type == fs::directory_file) {
        if (!fs::exists(path)) {
            try {
                fs::create_directories(path);
            }
            catch (...) {
                throw SystemError(path + " does not exist and could not be created");
            }
        }
        else if (!fs::is_directory(path)) {
            throw SystemError(path + " expected to exist and be a directory, but it is not");
        }
    }
    else if (!fs::exists(path)) {
        throw SystemError(path + " does not exist");
    }

    if (access(path.c_str(), mode) != 0) {
        std::ostringstream msg;
        msg << "Not enough permissions on " << path << " (Required " << requiredToString(mode) << ")";
        throw SystemError(msg.str());
    }
}


/// Check the environment is properly setup for FTS3 to run
static void runEnvironmentChecks()
{
    std::string urlCopyPath;
    if (!binaryExists("fts_url_copy", &urlCopyPath))
    {
        throw SystemError("Check if fts_url_copy process is set in the PATH env variable");
    }
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "fts_url_copy full path " << urlCopyPath << commit;

    std::string logsDir = ServerConfig::instance().get<std::string> ("ServerLogDirectory");
    std::string monDir = ServerConfig::instance().get<std::string> ("MessagingDirectory");

    checkPath(logsDir, R_OK, fs::directory_file);
    checkPath(monDir, R_OK | W_OK, fs::directory_file);
    checkPath(monDir + "/monitoring", R_OK | W_OK, fs::directory_file);
    checkPath(monDir + "/status", R_OK | W_OK, fs::directory_file);
    checkPath(monDir + "/stalled", R_OK | W_OK, fs::directory_file);
    checkPath(monDir + "/logs", R_OK | W_OK, fs::directory_file);
}


/// Prepare, fork and run FTS3
static void spawnServer(int argc, char** argv)
{
    ServerConfig::instance().read(argc, argv);
    std::string user = ServerConfig::instance().get<std::string>("User");
    std::string group = ServerConfig::instance().get<std::string>("Group");
    std::string pidDir = ServerConfig::instance().get<std::string>("PidDirectory");

    if (dropPrivileges(user, group)) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Changed running user and group to " << user << ":" << group << commit;
    }

    runEnvironmentChecks();

    bool isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");

    if (isDaemon) {
        int d = daemon(0, 0);
        if (d < 0) {
            throw SystemError("Can't fork the daemon");
        }
    }

    // Register PID file
    createPidFile(pidDir, "fts-server.pid");

    // Register signal handlers
    panic::setup_signal_handlers(shutdownCallback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Signal handlers installed" << commit;

    doServer();
}

/// Entry point
int main(int argc, char** argv)
{
    int n_running = countProcessesWithName("fts_server");
    if (n_running < 0) {
        std::cerr << "Could not check if FTS3 is already running" << std::endl;
        return EXIT_FAILURE;
    }
    else if (n_running > 1) {
        std::cerr << "Only one instance of FTS3 can run at the time"
                << std::endl;
        return EXIT_FAILURE;
    }

    try {
        spawnServer(argc, argv);
    }
    catch (const std::exception& ex) {
        std::cerr << "Failed to spawn the server! " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Failed to spawn the server! Unknown exception" << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

