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

#pragma once

#include <iostream>
#include <string>

#include "common/Logger.h"
#include "config/serverconfig.h"

#include "CleanMessageFiles.h"
#include "ProcessQueue.h"
#include "HeartBeat.h"
#include "OptimizerService.h"
#include "process_updater_db_service.h"
#include "process_multihop.h"
#include "process_reuse.h"
#include "process_service.h"
#include "transfer_web_service.h"


namespace fts3 {
namespace server {


/** Class representing the FTS3 server logic. */
class Server
{

public:

    /** Start the service. */
    void start()
    {
        boost::thread_group systemThreads;

        CleanMessageFiles cleanMessages;
        systemThreads.create_thread(boost::bind(&CleanMessageFiles::clean, cleanMessages));

        ProcessQueue queueHandler;
        systemThreads.create_thread(boost::bind(&ProcessQueue::processQueue, queueHandler));

        HeartBeat heartBeatHandler;
        systemThreads.create_thread(boost::bind(&HeartBeat::beat, heartBeatHandler));

        /*** Old style ***/
        if (!config::theServerConfig().get<bool> ("rush"))
            sleep(8);


        ProcessUpdaterDBService processUpdaterDBHandler;
        processUpdaterDBHandler.executeTransfer_p();

        /*wait for status updates to be processed and then start sanity threads*/
        if (!config::theServerConfig().get<bool> ("rush"))
            sleep(12);

        OptimizerService optimizerService;
        systemThreads.create_thread(boost::bind(&OptimizerService::runOptimizer, optimizerService));

        ProcessService processHandler;
        processHandler.executeTransfer_p();

        ProcessServiceReuse processReuseHandler;
        processReuseHandler.executeTransfer_p();

        ProcessServiceMultihop processMultihopHandler;
        processMultihopHandler.executeTransfer_p();

        unsigned int port = config::theServerConfig().get<unsigned int>("Port");
        const std::string& ip = config::theServerConfig().get<std::string>("IP");

        TransferWebService handler_t;
        handler_t.listen_p(port, ip);

        // Wait for all to finish
        ThreadPool::ThreadPool::instance().wait();
        systemThreads.join_all();
    }

    /* ---------------------------------------------------------------------- */

    /** Stop the service. */
    void stop()
    {
        ThreadPool::ThreadPool::instance().stop();
    }
};


/** Singleton instance of the FTS 3 server. */
inline Server& theServer()
{
    static Server s;
    return s;
}

} // end namespace server
} // end namespace fts3
