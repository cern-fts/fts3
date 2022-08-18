/*
 * Copyright (c) CERN 2017
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

#include "SupervisorService.h"
#include "config/ServerConfig.h"
#include "db/generic/SingleDbInstance.h"
#include "ThreadSafeList.h"
#include <msg-bus/events.h>

using namespace fts3::common;


namespace fts3 {
namespace server {


SupervisorService::SupervisorService(): BaseService("SupervisorService"),
    zmqContext(1), zmqPingSocket(zmqContext, ZMQ_SUB)
{
    std::string messagingDirectory = config::ServerConfig::instance().get<std::string>("MessagingDirectory");
    std::string address = std::string("ipc://") + messagingDirectory + "/url_copy-ping.ipc";
    zmqPingSocket.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    zmqPingSocket.bind(address.c_str());
}


void SupervisorService::runService()
{
    while (!boost::this_thread::interruption_requested()) {
        std::vector<fts3::events::MessageUpdater> events;
        zmq::message_t message;

        try {
            boost::this_thread::sleep(boost::posix_time::seconds(1));

            while (zmqPingSocket.recv(&message, ZMQ_NOBLOCK)) {
                fts3::events::MessageUpdater event;
                if (!event.ParseFromArray(message.data(), message.size())) {
                    continue;
                }
                events.emplace_back(event);

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Process Updater Monitor"
                                                << "\nJob id: " << event.job_id()
                                                << "\nFile id: " << event.file_id()
                                                << "\nPid: " << event.process_id()
                                                << "\nTimestamp: " << event.timestamp()
                                                << "\nThroughput: " << event.throughput()
                                                << "\nTransferred: " << event.transferred()
                                                << commit;

                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "[profiling:transfer]"
                                                << " file_id=" << event.file_id()
                                                << " timestamp=" << event.gfal_perf_timestamp()/1000
                                                << " inst_throughput=" << event.instantaneous_throughput()
                                                << " dif_transferred=" << event.transferred_since_last_ping()
                                                << " source_se=" << Uri::parse(event.source_surl()).getSeName()
                                                << " dest_se=" << Uri::parse(event.dest_surl()).getSeName()
                                                << commit;

                ThreadSafeList::get_instance().updateMsg(event);
            }

            if (!events.empty()) {
                db::DBSingleton::instance().getDBObjectInstance()->updateFileTransferProgressVector(events);
                events.clear();
            }
        }
        catch (const boost::thread_interrupted&) {
            FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Thread interruption requested" << commit;
            break;
        }
        catch (const std::exception &error) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << error.what() << commit;
        }
    }
}

}
}
