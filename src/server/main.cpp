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

#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "common/panic.h"
#include "config/serverconfig.h"
#include "db/generic/SingleDbInstance.h"
#include "profiler/Profiler.h"

#include "Server.h"


namespace fs = boost::filesystem;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server; 

const char *USER_NAME = "fts3";
const char *HOST_CERT = "/etc/grid-security/fts3hostcert.pem";
const char *HOST_KEY = "/etc/grid-security/fts3hostkey.pem";
const char *CONFIG_FILE = "/etc/fts3/fts3config";

extern bool stopThreads;


/// Initialize the database backend
/// @param test If set to true, do not use full pool size
static void intializeDatabase(bool test = false)
{
    std::string dbUserName = theServerConfig().get<std::string > ("DbUserName");
    std::string dbPassword = theServerConfig().get<std::string > ("DbPassword");
    std::string dbConnectString = theServerConfig().get<std::string > ("DbConnectString");
    int pooledConn = theServerConfig().get<int> ("DbThreadsNum");
    if(test)
        pooledConn = 2;

    db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, pooledConn);
}

/// Called by the signal handler
static void shutdownCallback(int signum, void*)
{
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")" << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    stopThreads = true;

    // Some require traceback
    switch (signum)
    {
        case SIGABRT: case SIGSEGV: case SIGILL:
        case SIGFPE: case SIGBUS: case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Stack trace: \n"
                << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size)
                << commit;
            break;
        default:
            break;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopping" << commit;

    if (!theServerConfig().get<bool> ("rush"))
        sleep(15);

    try {
        Server::instance().stop();
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closing" << commit;
        db::DBSingleton::destroy();
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS db connections closed" << commit;
    }
    catch (const std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception when forcing the database teardown: " << ex.what() << commit;
    }
    catch(...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unexpected exception when forcing the database teardown" << commit;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "FTS server stopped" << commit;

    // Handle termination for signals that do not imply errors
    // Signals that do imply an error (i.e. SIGSEGV) will trigger a coredump in panic.c
    switch (signum)
    {
        case SIGINT: case SIGTERM: case SIGUSR1:
            _exit(-signum);
    }
}

/// Main body of the FTS3 server
/// The configuration should have been loaded already
static void doServer(void)
{
    setenv("X509_USER_CERT", HOST_CERT, 1);
    setenv("X509_USER_KEY", HOST_KEY, 1);
    setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

    bool isDaemon = !theServerConfig().get<bool> ("no-daemon");
    std::string logPath = theServerConfig().get<std::string>("ServerLogDirectory");

    if (isDaemon && logPath.length() > 0) {
        logPath += "/fts3server.log";
        if (theLogger().redirect(logPath, logPath) != 0) {
            std::ostringstream msg;
            msg << "fts3 server failed to open log file " << logPath << " error is:" << strerror(errno);
            throw SystemError(msg.str());
        }
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Starting server..." << commit;

    intializeDatabase();
    fts3::ProfilingSubsystem::instance().start();
    Server::instance().start();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Server halt" << commit;
}

/// Validate the file or directory path exists, has the right type
/// and permissions
/// @param mode    as for access (R_OK, W_OK, ...)
/// @param type    file type
static void checkPath(const std::string& path, int mode, fs::file_type type)
{
    if (type == fs::directory_file) {
        if (!fs::is_directory(path)) {
            throw SystemError(path + " expected to exist and be a directory. Please, create it.");
        }
    }
    else {
        if (!fs::exists(path)) {
            throw SystemError(path + " does not exist");
        }
    }

    if (access(path.c_str(), mode) != 0) {
        std::ostringstream msg;
        msg << "Not enough permissions on " << path << " (Required " << std::ios_base::oct << mode << ")";
        throw SystemError(msg.str());
    }
}

/// Validate database availability and schema
void checkDbSchema()
{
    intializeDatabase(true);
    db::DBSingleton::instance().getDBObjectInstance()->checkSchemaLoaded();
    db::DBSingleton::destroy();
}

/// Check the environment is properly setup for FTS3 to run
static void runEnvironmentChecks()
{
    if (!fs::exists(CONFIG_FILE))
    {
        throw SystemError(std::string("fts3 server config file ") + CONFIG_FILE + " doesn't exist");
    }

    std::string urlCopyPath;
    if (!binaryExists("fts_url_copy", &urlCopyPath))
    {
        throw SystemError("Check if fts_url_copy process is set in the PATH env variable");
    }
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "fts_url_copy full path " << urlCopyPath << commit;

    if (!fs::exists(HOST_CERT))
    {
        throw SystemError("Check if hostcert/key are installed");
    }

    std::string logsDir = theServerConfig().get<std::string > ("ServerLogDirectory");

    checkPath("/etc/fts3", R_OK, fs::directory_file);
    checkPath(HOST_CERT, R_OK, fs::regular_file);
    checkPath(HOST_KEY, R_OK, fs::regular_file);
    checkPath(CONFIG_FILE, R_OK, fs::regular_file);
    checkPath(logsDir, R_OK, fs::directory_file);
    checkPath("/var/lib/fts3", R_OK | W_OK, fs::directory_file);
    checkPath("/var/lib/fts3/monitoring", R_OK | W_OK, fs::directory_file);
    checkPath("/var/lib/fts3/status", R_OK | W_OK, fs::directory_file);
    checkPath("/var/lib/fts3/stalled", R_OK | W_OK, fs::directory_file);
    checkPath("/var/lib/fts3/logs", R_OK | W_OK, fs::directory_file);

    checkDbSchema();
}

/// Change running UID
static void dropPrivileges()
{
    uid_t pw_uid = getUserUid(USER_NAME);
    setuid(pw_uid);
    seteuid(pw_uid);
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Process UID changed to " << pw_uid << commit;
}

/// Prepare, fork and run FTS3
static void spawnServer(int argc, char** argv)
{
    // Register signal handlers
    panic::setup_signal_handlers(shutdownCallback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Signal handlers installed" << commit;

    theServerConfig().read(argc, argv, true);
    dropPrivileges();
    runEnvironmentChecks();

    bool isDaemon = !theServerConfig().get<bool> ("no-daemon");

    if (isDaemon) {
        int d = daemon(0, 0);
        if (d < 0) {
            throw SystemError("Can't fork the daemon");
        }
    }

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

