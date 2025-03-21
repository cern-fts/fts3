/*
 * Copyright (c) CERN 2013-2025
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

#include <cstdlib>
#include <csignal>
#include <activemq/library/ActiveMQCPP.h>
#include <common/PidTools.h>

#include "common/ConcurrentQueue.h"
#include "common/DaemonTools.h"
#include "common/Exceptions.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "msg-bus/consumer.h"

#include "BrokerConfig.h"
#include "BrokerPublisher.h"
#include "MessageLoader.h"
#include "msg-bus/DirQ.h"

using namespace fts3::common;
using namespace fts3::config;

std::stop_source stop_source{};

void signal_handler(int signum)
{
    std::cout << "fts_activemq: signal " << signum << " received. Requesting stop..." << std::endl;
    if (stop_source.stop_requested()) {
        std::cout << "fts_activemq: was force killed" << std::endl;
        exit(EXIT_FAILURE);
    }
    stop_source.request_stop();
}

static void DoServer(bool isDaemon) throw()
{
    try {
        activemq::library::ActiveMQCPP::initializeLibrary();
    } catch (const std::runtime_error &err) {
        FTS3_COMMON_LOGGER_LOG(CRIT, "fts_activemq: error while loading the ActiveMQCPP library!");
        return;
    }

    try {
        BrokerConfig config(ServerConfig::instance().get<std::string>("MonitoringConfigFile"));

        std::string logFile = config.GetLogFilePath();
        if (isDaemon) {
            if (theLogger().redirect(logFile, logFile) != 0) {
                FTS3_COMMON_LOGGER_LOG(CRIT, "Could not open the log file");
                return;
            }
        }

        theLogger().setLogLevel(Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel")));
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        //Initialize here to avoid race conditions
        ConcurrentQueue<std::unique_ptr<std::vector<MonitoringMessage>>>::getInstance().set_stop_token(stop_source.get_token());

        auto msg_directory = ServerConfig::instance().get<std::string>("MessagingDirectory");
        MessageLoader messageLoader(msg_directory, stop_source.get_token());
        MessageRemover messageRemover(msg_directory);
        BrokerPublisher brokerPublisher(config, messageRemover, stop_source.get_token());
        std::jthread messageLoaderThread(&MessageLoader::start, &messageLoader);
        std::jthread messageRemoverThread([&messageRemover](std::stop_token s) {messageRemover.start(s);} );
        std::jthread brokerPublisherThread(&BrokerPublisher::start, &brokerPublisher);

        FTS3_COMMON_LOGGER_LOG(INFO, "Started MessageLoaderThread, MessageRemoverThread, BrokerPublisherThread!");
        if (messageLoaderThread.joinable()) {
            messageLoaderThread.join();
        }

        if (brokerPublisherThread.joinable()) {
            brokerPublisherThread.join();
        }

        // Once the brokerPublisherThread exit, this guarantees that no more messages will be moved to the MessageRemoverThread for deletion.
        // Then, we let the MessageRemover thread collect and remove all remaining messages, this is necessary to avoid sending duplicate
        // messages on the next fts_activemq daemon start.
        if (messageRemoverThread.joinable()) {
            FTS3_COMMON_LOGGER_LOG(INFO, "Waitig on messageRemoverThread to exit!");
            messageRemoverThread.request_stop();
            messageRemoverThread.join();
        }
        FTS3_COMMON_LOGGER_LOG(INFO, "All threads finished, exiting...");
    } catch (cms::CMSException& e) {
        std::string errorMessage = "PROCESS_ERROR " + e.getStackTraceString();
        FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
    } catch (const std::exception& e) {
        std::string  errorMessage = "PROCESS_ERROR " + std::string(e.what());
        FTS3_COMMON_LOGGER_LOG(ERR, errorMessage);
    } catch (...) {
        FTS3_COMMON_LOGGER_LOG(ERR, "PROCESS_ERROR Unknown exception");
    }

    activemq::library::ActiveMQCPP::shutdownLibrary();
}


static void spawnServer(int argc, char **argv)
{
    ServerConfig::instance().read(argc, argv);
    std::string user = ServerConfig::instance().get<std::string>("User");
    std::string group = ServerConfig::instance().get<std::string>("Group");
    std::string pidDir = ServerConfig::instance().get<std::string>("PidDirectory");

    if (dropPrivileges(user, group)) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Changed running user and group to " << user << ":" << group << commit;
    }

    bool isDaemon = !ServerConfig::instance().get<bool> ("no-daemon");
    if (isDaemon) {
        int d = daemon(0, 0);
        if (d < 0) {
            throw SystemError("Can't set daemon");
        }
    }

    createPidFile(pidDir, "fts-activemq.pid");

    DoServer(isDaemon);
}


int main(int argc, char **argv)
{
    pthread_setname_np(pthread_self(), "fts_activemq");
    // Do not even try if already running
    int n_running = countProcessesWithName("fts_activemq");
    if (n_running < 0) {
        std::cerr << "Could not check if FTS3 is already running" << std::endl;
        return EXIT_FAILURE;
    } else if (n_running > 1) {
        std::cerr << "Only 1 instance of FTS3 messaging daemon can run at a time" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        spawnServer(argc, argv);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to spawn the messaging server! " << ex.what() << std::endl;
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "Failed to spawn the messaging server! Unknown exception" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
