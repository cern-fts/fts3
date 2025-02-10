/*
* Copyright (c) CERN 2024
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

#include <errno.h>
#include <signal.h>

#include "common/panic.h"
#include "common/Logger.h"
#include "common/PidTools.h"
#include "common/Exceptions.h"
#include "common/DaemonTools.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "TokenServer.h"

using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::token;


/// Called by the signal handler
static void shutdownCallback(int signum, void*)
{
    FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Caught signal " << signum
                                    << " (" << strsignal(signum) << ")"
                                    << commit;
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Future signals will be ignored!" << commit;

    TokenServer::instance().stop();
    FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Daemon stopping" << commit;

    // Some require traceback
    switch (signum) {
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

/// Run the Token server
static void doServer()
{
    auto isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");
    auto logPath = ServerConfig::instance().get<std::string>("ServerLogDirectory");

    if (isDaemon && !logPath.empty()) {
        logPath += "/fts3token.log";

        if (theLogger().redirect(logPath, logPath) != 0) {
            std::ostringstream msg;
            msg << "fts3 Token daemon failed to open logfile=" << logPath << " error=" << strerror(errno);
            throw SystemError(msg.str());
        }
    }

    theLogger().setLogLevel(Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel")));

    initializeDatabase();
    TokenServer::instance().start();
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Daemon started" << commit;

    TokenServer::instance().wait();
    FTS3_COMMON_LOGGER_NEWLOG(INFO)<< "Daemon halt" << commit;
}

/// Prepare, fork and run the Token daemon
static void spawnServer(int argc, char** argv)
{
    ServerConfig::instance().read(argc, argv);

    auto user = ServerConfig::instance().get<std::string>("User");
    auto group = ServerConfig::instance().get<std::string>("Group");
    auto pidDir = ServerConfig::instance().get<std::string>("PidDirectory");
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "[user] " << user << " [group] " << group << " [pidDir] " << pidDir << commit;

    if (dropPrivileges(user, group)) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Changed running user and group to " << user << ":" << group << commit;
    }

	bool isDaemon = !ServerConfig::instance().get<bool>("no-daemon");

    if (isDaemon) {
        int d = daemon(0, 0);
        std::cerr << "Created the fts_token daemon!" << std::endl;

        if (d < 0) {
        	std::cerr << "Can't fork the daemon " << d << std::endl;
            throw SystemError("Can't fork the token daemon!");
        }
    }

    createPidFile(pidDir, "fts-token.pid");

    panic::setup_signal_handlers(shutdownCallback, NULL);
    FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Signal handlers installed" << commit;

    doServer();
}

/// Entry point
int main(int argc, char** argv)
{
    // Ensure only one process is running
    int countRunning = countProcessesWithName("fts_token");

    if (countRunning < 0) {
        std::cerr << "Could not check if the Token daemon is already running!" << std::endl;
        return EXIT_FAILURE;
    }

    if (countRunning > 1) {
        std::cerr << "Only one instance of the Token daemon can run at one time!" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        spawnServer(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to spawn the Token daemon! "
                  << "(exception=" << ex.what() << ")" << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Failed to spawn the Token daemon! (unknown exception)" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
