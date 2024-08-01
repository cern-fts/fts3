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
#include <common/PidTools.h>

#include "config/ServerConfig.h"
#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "common/panic.h"
#include "db/generic/SingleDbInstance.h"

#include "QoSServer.h"


using namespace fts3::common;
using namespace fts3::config;
namespace fs = boost::filesystem;


const char *HOST_CERT = "/etc/grid-security/fts3hostcert.pem";
const char *HOST_KEY = "/etc/grid-security/fts3hostkey.pem";


/// Called by the signal handler
static void shutdownCallback(int signum, void*)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")"
                                    << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Future signals will be ignored!" << commit;

    QoSServer::instance().stop();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Daemon stopping" << commit;

    // Some require traceback
    switch (signum)
    {
        case SIGABRT: case SIGSEGV: case SIGILL:
        case SIGFPE: case SIGBUS: case SIGTRAP:
        case SIGSYS:
            FTS3_COMMON_LOGGER_NEWLOG(CRIT)<< "Stack trace: \n" << panic::stack_dump(panic::stack_backtrace, panic::stack_backtrace_size) << commit;
            break;
        default:
            break;
    }
}


/// Initialize the database backend
static void initializeDatabase()
{
    std::string dbType = ServerConfig::instance().get<std::string > ("DbType");
    std::string dbUserName = ServerConfig::instance().get<std::string>("DbUserName");
    std::string dbPassword = ServerConfig::instance().get<std::string>("DbPassword");
    std::string dbConnectString = ServerConfig::instance().get<std::string>("DbConnectString");

    db::DBSingleton::instance().getDBObjectInstance()->init(dbType, dbUserName, dbPassword, dbConnectString, 8);
}


static void gfal2LogCallback(const gchar*, GLogLevelFlags, const gchar* message, gpointer)
{
    if (message) {
        const char* prefix = "Gfal2: ";

        if (strncmp(message, "Davix: ", 7) == 0) {
            prefix = "";
        }

        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << prefix << message << fts3::common::commit;
    }
}


/// Run the Bring Online server
static void doServer()
{
    bool isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");
    std::string logPath = ServerConfig::instance().get<std::string>("ServerLogDirectory");
    if (isDaemon && !logPath.empty()) {
        logPath += "/fts3qos.log";
        if (theLogger().redirect(logPath, logPath) != 0) {
            std::ostringstream msg;
            msg << "fts3 qosdaemon failed to open log file " << logPath << " error is:" << strerror(errno);
            throw SystemError(msg.str());
        }
    }

    auto logLevel = Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel"));
    theLogger().setLogLevel(logLevel);

    if (logLevel <= Logger::LogLevel::DEBUG) {
        setenv("XRD_LOGLEVEL", "Debug", 1);
        gfal2_log_set_level(G_LOG_LEVEL_DEBUG);
        gfal2_log_set_handler((GLogFunc) gfal2LogCallback, NULL);
    }

    initializeDatabase();
    QoSServer::instance().start();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Daemon started" << commit;

    QoSServer::instance().wait();

    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Daemon halt" << commit;
}

/// Prepare, fork and run qos daemon
static void spawnServer(int argc, char** argv)
{
    ServerConfig::instance().read(argc, argv);

    std::string user = ServerConfig::instance().get<std::string>("User");
    std::string group = ServerConfig::instance().get<std::string>("Group");
    std::string pidDir = ServerConfig::instance().get<std::string>("PidDirectory");
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "[user] " << user << " [group] " << group << " [pidDir] " << pidDir << commit;

    if (dropPrivileges(user, group)) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Changed running user and group to " << user << ":" << group << commit;
    }

	bool isDaemon = !ServerConfig::instance().get<bool>("no-daemon");
    if (isDaemon) {
        int d = daemon(0, 0);
        std::cerr << "Created the qos_daemon" << std::endl;
        if (d < 0) {
        	std::cerr << "Can't fork the daemon " << d << std::endl;
            throw SystemError("Can't fork the daemon");
        }
    }
    createPidFile(pidDir, "fts-qos.pid");

    panic::setup_signal_handlers(shutdownCallback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Signal handlers installed" << commit;

    // Watch for changes on the config file
    ServerConfig::instance().startMonitor();

    doServer();
}

/// Entry point
int main(int argc, char** argv)
{
    // Do not even try if already running
    int n_running = countProcessesWithName("fts_qos");
    std::cerr << "Counting processes:" << n_running << std::endl;
    if (n_running < 0) {
        std::cerr << "Could not check if QoS daemon is already running" << std::endl;
        return EXIT_FAILURE;
    }
    else if (n_running > 1) {
        std::cerr << "Only one instance of QoS daemon can run at the time" << std::endl;
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
