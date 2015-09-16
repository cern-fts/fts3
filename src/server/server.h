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

/** \file server.h FTS3 server class interface. */

#pragma once

#include "generic_server.h"
#include "heartbeat.h"
#include "heartbeatActive.h"
#include "threadpool.h"
#include "transfer_web_service.h"
#include "process_service.h"
#include "process_updater_db_service.h"
#include "ProcessQueue.h"
#include "process_reuse.h"
#include "process_multihop.h"
#include "cleanStateLogs.h"

namespace fts3 {
namespace server {

/* -------------------------------------------------------------------------- */

struct ServerTraits
{
    typedef TransferWebService TransferWebServiceType;
    typedef ProcessService ProcessServiceType;
    typedef ProcessServiceReuse ProcessServiceReuseType;
    typedef ProcessServiceMultihop ProcessServiceMultihopType;
    typedef ProcessUpdaterDBService ProcessUpdaterDBServiceType;
    typedef ProcessQueue ProcessQueueType;
    typedef ThreadPool::ThreadPool ThreadPoolType;
    typedef HearBeat HeartBeatType;
    typedef HearBeatActive HeartBeatTypeActive;
    typedef CleanLogsActive CleanLogsTypeActive;
};

/* -------------------------------------------------------------------------- */

typedef GenericServer<ServerTraits> Server;

/* -------------------------------------------------------------------------- */

/** Singleton instance of the FTS 3 server. */
inline Server& theServer()
{
    static Server s;
    return s;
}

} // end namespace server
} // end namespace fts3

