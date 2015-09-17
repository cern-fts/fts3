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

#include "Server.h"

#include "common/Logger.h"
#include "config/serverconfig.h"

#include "ProcessQueue.h"
#include "HeartBeat.h"
#include "services/optimizer/OptimizerService.h"
#include "ProcessUpdaterDbService.h"
#include "ProcessServiceMultihop.h"
#include "ProcessServiceReuse.h"
#include "ProcessService.h"
#include "services/cleaner/CleanerService.h"
#include "WebService.h"


namespace fts3 {
namespace server {


void Server::start()
{
    CleanerService cleanMessages;
    systemThreads.create_thread(boost::ref(cleanMessages));

    ProcessQueue queueHandler;
    systemThreads.create_thread(boost::ref(queueHandler));

    HeartBeat heartBeatHandler;
    systemThreads.create_thread(boost::ref(heartBeatHandler));

    if (!config::theServerConfig().get<bool> ("rush"))
        sleep(8);

    ProcessUpdaterDBService processUpdaterDBHandler;
    systemThreads.create_thread(boost::ref(processUpdaterDBHandler));

    /*wait for status updates to be processed and then start sanity threads*/
    if (!config::theServerConfig().get<bool> ("rush"))
        sleep(12);

    OptimizerService optimizerService;
    systemThreads.create_thread(boost::ref(optimizerService));

    ProcessService processHandler;
    systemThreads.create_thread(boost::ref(processHandler));

    ProcessServiceReuse processReuseHandler;
    systemThreads.create_thread(boost::ref(processReuseHandler));

    ProcessServiceMultihop processMultihopHandler;
    systemThreads.create_thread(boost::ref(processMultihopHandler));

    unsigned int port = config::theServerConfig().get<unsigned int>("Port");
    const std::string& ip = config::theServerConfig().get<std::string>("IP");

    WebService webServiceHandler(port, ip);
    systemThreads.create_thread(boost::ref(webServiceHandler));

    // Wait for all to finish
    systemThreads.join_all();
}


void Server::stop()
{
    systemThreads.interrupt_all();
}

} // end namespace server
} // end namespace fts3
