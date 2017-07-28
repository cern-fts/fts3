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

#include "SingleTrStateInstance.h"
#include <sstream>

#include <glib.h>

#include "common/Exceptions.h"
#include "config/ServerConfig.h"
#include "common/Logger.h"
#include "db/generic/SingleDbInstance.h"
#include "msg-bus/producer.h"

using namespace db;
using namespace fts3::common;
using namespace fts3::config;
using namespace fts3::server;


std::unique_ptr<SingleTrStateInstance> SingleTrStateInstance::i;
boost::mutex SingleTrStateInstance::_mutex;


// Implementation

SingleTrStateInstance::SingleTrStateInstance(): monitoringMessages(true)
{
    std::string monitoringMessagesStr = ServerConfig::instance().get<std::string> ("MonitoringMessaging");
    if(monitoringMessagesStr == "false")
        monitoringMessages = false;

    ftsAlias = ServerConfig::instance().get<std::string>("Alias");
}

SingleTrStateInstance::~SingleTrStateInstance()
{
}


void SingleTrStateInstance::sendStateMessage(const std::string& jobId, uint64_t fileId)
{
    if (!monitoringMessages)
        return;

    if (!producer.get()) {
        producer.reset(new Producer(ServerConfig::instance().get<std::string>("MessagingDirectory")));
    }

    std::vector<TransferState> files;
    try {
        files = db::DBSingleton::instance().getDBObjectInstance()->getStateOfTransfer(jobId, fileId);
        if (!files.empty()) {
            for (auto it = files.begin(); it != files.end(); ++it) {
                MsgIfce::getInstance()->SendTransferStatusChange(*producer, *it);
            }
        }
    }
    catch (BaseException &e) {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state, " << e.what() << commit;
    }
    catch (std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state, " << ex.what() << commit;
    }
    catch (...) {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Failed saving transfer state " << commit;
    }
}
