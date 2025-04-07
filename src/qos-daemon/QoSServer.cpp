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


#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "server/common/DrainMode.h"
#include "server/common/BaseService.h"
#include "server/services/heartbeat/HeartBeat.h"

#include "QoSServer.h"
#include "task/PollTask.h"
#include "task/HttpPollTask.h"
#include "task/ArchivingPollTask.h"
#include "task/WaitingRoom.h"
#include "fetch/FetchStaging.h"
#include "fetch/FetchCancelStaging.h"
#include "fetch/FetchArchiving.h"
#include "fetch/FetchCancelArchiving.h"
#include "state/StagingStateUpdater.h"

using namespace fts3::common;
using namespace fts3::config;


QoSServer::QoSServer(): threadpool(10)
{
    FTS3_COMMON_LOGGER_NEWLOG(TRACE) << "QoS server created" << commit;
}


QoSServer::~QoSServer()
{
    stop();
    wait();
}


void QoSServer::start()
{
    setenv("GLOBUS_THREAD_MODEL", "pthread", 1);

    auto logLevel = Logger::getLogLevel(ServerConfig::instance().get<std::string>("LogLevel"));
    bool debugLogging = (logLevel <= Logger::LogLevel::DEBUG);
    Gfal2Task::createPrototype(debugLogging);

    FetchStaging fs(threadpool);
    FetchCancelStaging fcs(threadpool);
    FetchArchiving fa(threadpool);
    FetchCancelArchiving fca(threadpool);

    waitingRoom.attach(threadpool);
    httpWaitingRoom.attach(threadpool);
    archivingWaitingRoom.attach(threadpool);

    systemThreads.create_thread(boost::bind(&WaitingRoom<PollTask>::run, &waitingRoom));
    systemThreads.create_thread(boost::bind(&WaitingRoom<HttpPollTask>::run, &httpWaitingRoom));
    systemThreads.create_thread(boost::bind(&WaitingRoom<ArchivingPollTask>::run, &archivingWaitingRoom));

    // HeartBeat
    auto heartBeatService = std::make_shared<fts3::server::HeartBeat>(processName);
    systemThreads.create_thread(std::bind(&fts3::server::BaseService::runService, heartBeatService));

    // Give heartbeat some time to be processed
    if (!ServerConfig::instance().get<bool>("rush")) {
        boost::this_thread::sleep(boost::posix_time::seconds(8));
    }

    // Staging
    systemThreads.create_thread(boost::bind(&FetchStaging::fetch, fs));
    systemThreads.create_thread(boost::bind(&FetchCancelStaging::fetch, fcs));
    // Archiving
    systemThreads.create_thread(boost::bind(&FetchArchiving::fetch, fa));
    systemThreads.create_thread(boost::bind(&FetchCancelArchiving::fetch, fca));
    // StateUpdaters
    systemThreads.create_thread(boost::bind(&StagingStateUpdater::run, &stagingStateUpdater));
    systemThreads.create_thread(boost::bind(&ArchivingStateUpdater::run, &archivingStateUpdater));
}


void QoSServer::wait()
{
    systemThreads.join_all();
    threadpool.join();
}


void QoSServer::stop()
{
    stagingStateUpdater.recover();
    archivingStateUpdater.recover();
    threadpool.interrupt();
    systemThreads.interrupt_all();
}
