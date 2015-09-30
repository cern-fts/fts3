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

#include <boost/filesystem.hpp>

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "common/panic.h"
#include "common/ThreadPool.h"


#include "task/Gfal2Task.h"
#include "fetch/FetchStaging.h"
#include "fetch/FetchCancelStaging.h"
#include "fetch/FetchDeletion.h"
#include "state/StagingStateUpdater.h"
#include "state/DeletionStateUpdater.h"
#include "server/DrainMode.h"


using namespace fts3::common;
using namespace fts3::config;
namespace fs = boost::filesystem;


bool stopThreads = false;
const char *HOST_CERT = "/etc/grid-security/fts3hostcert.pem";
const char *HOST_KEY = "/etc/grid-security/fts3hostkey.pem";
const char *CONFIG_FILE = "/etc/fts3/fts3config";
const char *USER_NAME = "fts3";


/// Called by the signal handler
static void shutdownCallback(int signum, void*)
{
    StagingStateUpdater::instance().recover();
    DeletionStateUpdater::instance().recover();


    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")"
                                    << commit;
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Future signals will be ignored!" << commit;

    stopThreads = true;

    // Some require traceback
    switch (signum)
    {
        case SIGABRT: case SIGSEGV: case SIGILL:
        case SIGFPE: case SIGBUS: case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Stack trace: \n" << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size) << commit;
            break;
        default:
            break;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Daemon stopping" << commit;

    try {
        db::DBSingleton::destroy();
    }
    catch (const std::exception& ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Exception when forcing the database teardown: " << ex.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)<< "Unexpected exception when forcing the database teardown" << commit;
    }

    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Daemon stopped" << commit;

    // Handle termination for signals that do not imply errors
    // Signals that do imply an error (i.e. SIGSEGV) will trigger a coredump in panic.c
    switch (signum)
    {
        case SIGINT: case SIGTERM: case SIGUSR1:
            _exit(-signum);
    }
}


/// Initialize the database backend
static void initializeDatabase()
{
    std::string dbUserName = ServerConfig::instance().get<std::string>("DbUserName");
    std::string dbPassword = ServerConfig::instance().get<std::string>("DbPassword");
    std::string dbConnectString = ServerConfig::instance().get<std::string>("DbConnectString");

    db::DBSingleton::instance().getDBObjectInstance()->init(dbUserName, dbPassword, dbConnectString, 8);
}

/// Run in the background updating the status of this server
/// This is a thread! Do not let exceptions exit the scope
static void heartBeat(void)
{
    unsigned myIndex=0, count=0;
    unsigned hashStart=0, hashEnd=0;
    const std::string service_name = "fts_bringonline";

    while (!stopThreads) {
        try {
            //check if draining is on
            if (fts3::server::DrainMode::instance()) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Set to drain mode, no more checking stage-in files for this instance!" << commit;
                sleep(15);
                continue;
            }

            db::DBSingleton::instance().getDBObjectInstance()->updateHeartBeat(
                &myIndex, &count, &hashStart, &hashEnd, service_name);

            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Systole: host " << myIndex << " out of " << count
            << " [" << std::hex << hashStart << ':' << std::hex << hashEnd << ']'
            << std::dec
            << commit;

            sleep(60);
        }
        catch (std::exception& ex)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            sleep(2);
        }
        catch (...)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Unhandled exception" << commit;
            sleep(2);
        }
    }
}

/// Main body of the bring online daemon
/// The configuration should have been loaded already
static void doServer(void)
{
    setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Starting daemon..." << commit;

    initializeDatabase();
    fts3::ProfilingSubsystem::instance().start();

    boost::thread heartBeatThread(heartBeat);

    std::string infosys = ServerConfig::instance().get<std::string>("Infosys");
    Gfal2Task::createPrototype (infosys);

    fts3::common::ThreadPool<Gfal2Task> threadpool(10);
    FetchStaging fs(threadpool);
    FetchCancelStaging fcs(threadpool);
    FetchDeletion fd(threadpool);

    boost::thread_group gr;
    gr.create_thread(boost::bind(&FetchStaging::fetch, fs));
    gr.create_thread(boost::bind(&FetchCancelStaging::fetch, fcs));
    gr.create_thread(boost::bind(&FetchDeletion::fetch, fd));
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Daemon started..." << commit;

    gr.join_all();
}

/// Check the environment is properly setup for FTS3 to run
static void runEnvironmentChecks()
{
    if (!fs::exists(CONFIG_FILE)) {
        throw SystemError(std::string("fts3 server config file ") + CONFIG_FILE + " doesn't exist");
    }
}

/// Change running UID
static void dropPrivileges()
{
    uid_t pw_uid = getUserUid(USER_NAME);
    setuid(pw_uid);
    seteuid(pw_uid);
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Process UID changed to " << pw_uid << commit;
}

/// Prepare, fork and run bring online daemon
static void spawnServer(int argc, char** argv)
{
    panic::setup_signal_handlers(shutdownCallback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Signal handlers installed" << commit;

    ServerConfig::instance().read(argc, argv);
    dropPrivileges();
    runEnvironmentChecks();

    bool isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");

    if (isDaemon) {
        std::string logPath = ServerConfig::instance().get<std::string>("ServerLogDirectory");
        if (!logPath.empty()) {
            logPath += "/fts3bringonline.log";
            if (theLogger().redirect(logPath, logPath) != 0) {
                std::ostringstream msg;
                msg << "fts3 server failed to open log file " << logPath << " error is:" << strerror(errno);
                throw SystemError(msg.str());
            }
        }

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
    // Do not even try if already running
    int n_running = countProcessesWithName("fts_bringonline");
    if (n_running < 0) {
        std::cerr << "Could not check if FTS3 is already running" << std::endl;
        return EXIT_FAILURE;
    }
    else if (n_running > 1) {
        std::cerr << "Only one instance of FTS3 can run at the time" << std::endl;
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
