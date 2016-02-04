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

#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include "common/Logger.h"
#include "consumer.h"


Consumer::Consumer(const std::string &baseDir, unsigned limit):
    baseDir(baseDir), limit(limit),
    monitoringQueue(baseDir + "/monitoring"), statusQueue(baseDir + "/status"),
    stalledQueue(baseDir + "/stalled"), logQueue(baseDir + "/logs"),
    stagingQueue(baseDir + "/staging"), deletionQueue(baseDir + "/deletion")
{
}


Consumer::~Consumer()
{
}


template <typename MSG>
static int genericConsumer(DirQ &dirq, unsigned limit, std::vector<MSG> &messages)
{
    MSG event;

    for (auto iter = dirq_first(dirq); iter != NULL && limit > 0; iter = dirq_next(dirq), --limit) {
        if (dirq_lock(dirq, iter, 0) == 0) {
            const char *path = dirq_get_path(dirq, iter);

            try {
                std::ifstream fstream(path);
                event.ParseFromIstream(&fstream);
            }
            catch (const std::exception &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                    << "Could not load message from " << path << " (" << ex.what() << ")"
                    << fts3::common::commit;
            }

            messages.emplace_back(event);
            dirq_remove(dirq, iter);
        }
    }

    return 0;
}


int Consumer::runConsumerStatus(std::vector<fts3::events::Message> &messages)
{
    return genericConsumer<fts3::events::Message>(statusQueue, limit, messages);
}


int Consumer::runConsumerStall(std::vector<fts3::events::MessageUpdater> &messages)
{
    return genericConsumer<fts3::events::MessageUpdater>(stalledQueue, limit, messages);
}


int Consumer::runConsumerLog(std::map<int, fts3::events::MessageLog> &messages)
{
    fts3::events::MessageLog buffer;

    for (auto iter = dirq_first(logQueue); iter != NULL && limit > 0; iter = dirq_next(logQueue), --limit) {
        if (dirq_lock(logQueue, iter, 0) == 0) {
            const char *path = dirq_get_path(logQueue, iter);

            try {
                std::ifstream fstream(path);
                buffer.ParseFromIstream(&fstream);
            }
            catch (const std::exception &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Could not load message from " << path << " (" << ex.what() << ")"
                << fts3::common::commit;
            }

            messages[buffer.file_id()] = buffer;
            dirq_remove(logQueue, iter);
        }
    }

    return 0;
}


int Consumer::runConsumerDeletions(std::vector<fts3::events::MessageBringonline> &messages)
{
    return genericConsumer<fts3::events::MessageBringonline>(deletionQueue, limit, messages);
}


int Consumer::runConsumerStaging(std::vector<fts3::events::MessageBringonline> &messages)
{
    return genericConsumer<fts3::events::MessageBringonline>(stagingQueue, limit, messages);
}


int Consumer::runConsumerMonitoring(std::vector<std::string> &messages)
{
    std::string content;

    for (auto iter = dirq_first(monitoringQueue); iter != NULL && limit > 0; iter = dirq_next(monitoringQueue), --limit) {
        if (dirq_lock(monitoringQueue, iter, 0) == 0) {
            const char *path = dirq_get_path(monitoringQueue, iter);

            try {
                std::ifstream fstream(path);
                content.assign((std::istreambuf_iterator<char>(fstream)), std::istreambuf_iterator<char>());
                messages.emplace_back(content);
            }
            catch (const std::exception &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)
                << "Could not load message from " << path << " (" << ex.what() << ")"
                << fts3::common::commit;
            }


            dirq_remove(monitoringQueue, iter);
        }
    }

    return 0;
}


void Consumer::purgeAll()
{
    dirq_purge(monitoringQueue);
    dirq_purge(statusQueue);
    dirq_purge(stalledQueue);
    dirq_purge(logQueue);
    dirq_purge(stagingQueue);
    dirq_purge(deletionQueue);
}
