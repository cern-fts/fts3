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

#include <fstream>
#include "common/Logger.h"
#include "common/ConcurrentQueue.h"
#include "MonitoringMessage.h"
#include "msg-bus/DirQ.h"
#include "MessageLoader.h"

using fts3::common::ConcurrentQueue;
using namespace fts3::common;

MessageLoader::MessageLoader(const std::string &baseDir, std::stop_token token) :
                             monitoringQueue(std::make_unique<DirQ>(baseDir + "/monitoring")), stop_token(token)
{
}

MessageLoader::~MessageLoader()
{
}

int MessageLoader::loadMonitoringMessages()
{
    const char *error = NULL;
    dirq_clear_error(*monitoringQueue);

    const unsigned int batch_size = 5000;
    std::unique_ptr<std::vector<MonitoringMessage>> current_vector = std::make_unique<std::vector<MonitoringMessage>>();
    current_vector->reserve(batch_size);
    for (const char *iter = dirq_first(*monitoringQueue); iter != nullptr; iter = dirq_next(*monitoringQueue)) {
        auto& concurrentQueue = fts3::common::ConcurrentQueue<std::unique_ptr<std::vector<MonitoringMessage>>>::getInstance();
        if (stop_token.stop_requested()) {
            return 0;
        }

        if (strcmp(iter, last_message.c_str()) <= 0) {
            continue;
        }

        if (current_vector->size() >= batch_size) {
            if (concurrentQueue.size() > 10) {
                concurrentQueue.drain();
            }
            concurrentQueue.push(std::move(current_vector));
            current_vector = std::make_unique<std::vector<MonitoringMessage>>();
            current_vector->reserve(batch_size);
        }

        if (dirq_lock(*monitoringQueue, iter, 0)) {
            if (dirq_unlock(*monitoringQueue, iter, 0) && dirq_lock(*monitoringQueue, iter, 0)) {
                break;
            }
        }

        const char *path = dirq_get_path(*monitoringQueue, iter);
        try {
            std::ifstream fstream{path};
            std::string content{(std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>()};
            std::string iterator{iter};

            current_vector->emplace_back(std::move(content), std::move(iterator));
            last_message = iter;
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load message from " << path << " (" << ex.what() << ")"
                                           << fts3::common::commit;
        }

        if (dirq_unlock(*monitoringQueue, iter, 0)) {
            break;
        }
    }

    if (!current_vector->empty()) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << "Pushing vector to concurrent queue:" << current_vector->size() << " elements" << fts3::common::commit;
        fts3::common::ConcurrentQueue<std::unique_ptr<std::vector<MonitoringMessage>>>::getInstance().push(std::move(current_vector));
    }

    error = dirq_get_errstr(*monitoringQueue);
    if (error) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to consume messages: " << error << fts3::common::commit;
        return -1;
    }

    return 0;
}

void MessageLoader::start()
{
    std::condition_variable_any cv;
    std::mutex mtx;
    std::unique_lock lock(mtx);
    while (!stop_token.stop_requested()) {
        try {
            if (loadMonitoringMessages()) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "MessageLoader: unable to consume DirQ monitoring messages" << commit;
            }
        } catch (const std::exception &ex) {
            FTS3_COMMON_LOGGER_LOG(ERR, "Unable to load monitoring messages" << ex.what()) << commit;
        } catch (...) {
            FTS3_COMMON_LOGGER_LOG(CRIT, "Unexpected exception");
        }
        cv.wait_for(lock, stop_token, std::chrono::seconds(30), []{ return false; });
        dirq_purge(*monitoringQueue);
    }
    FTS3_COMMON_LOGGER_LOG(DEBUG, "MessageLoader exited!");
}
