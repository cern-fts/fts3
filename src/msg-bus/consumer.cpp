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

#include <dirq.h>
#include <fstream>
#include <cstring>
#include "common/Logger.h"
#include "consumer.h"
#include "DirQ.h"


Consumer::Consumer(const std::string &baseDir, unsigned limit): baseDir(baseDir), limit(limit),
    statusQueue(std::make_unique<DirQ>(baseDir + "/status")), stalledQueue(std::make_unique<DirQ>(baseDir + "/stalled")),
    logQueue(std::make_unique<DirQ>(baseDir + "/logs")), stagingQueue(std::make_unique<DirQ>(baseDir + "/staging")),
    deletionQueue(std::make_unique<DirQ>(baseDir + "/deletion"))
{
}

Consumer::~Consumer()
{
}


template <typename MSG>
static int genericConsumer(std::unique_ptr<DirQ> &dirq, unsigned limit, std::vector<MSG> &messages)
{
    MSG event;

    const char *error = NULL;
    dirq_clear_error(*dirq);

    unsigned i = 0;
    for (auto iter = dirq_first(*dirq); iter != NULL && i < limit; iter = dirq_next(*dirq), ++i) {
        if (dirq_lock(*dirq, iter, 0) == 0) {
            const char *path = dirq_get_path(*dirq, iter);

            try {
                std::ifstream fstream(path);
                event.ParseFromIstream(&fstream);
            } catch (const std::exception &ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not load message from " << path << " (" << ex.what() << ")"
                                               << fts3::common::commit;
            }

            messages.emplace_back(event);
            if (dirq_remove(*dirq, iter) < 0) {
                error = dirq_get_errstr(*dirq);
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to remove message from queue (" << path << "): "
                                               << error << fts3::common::commit;
                dirq_clear_error(*dirq);
            }
        }
    }

    error = dirq_get_errstr(*dirq);
    if (error) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to consume messages: " << error << fts3::common::commit;
        return -1;
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

    const char *error = NULL;
    dirq_clear_error(*logQueue);

    unsigned i = 0;
    for (auto iter = dirq_first(*logQueue); iter != NULL && i < limit; iter = dirq_next(*logQueue), ++i) {
        if (dirq_lock(*logQueue, iter, 0) == 0) {
            const char *path = dirq_get_path(*logQueue, iter);

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
            if (dirq_remove(*logQueue, iter) < 0) {
                error = dirq_get_errstr(*logQueue);
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to remove message from queue (" << path << "): "
                    << error
                    << fts3::common::commit;
                dirq_clear_error(*logQueue);
            }
        }
    }

    error = dirq_get_errstr(*logQueue);
    if (error) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Failed to consume messages: " << error << fts3::common::commit;
        return -1;
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


static void _purge(DirQ *dq)
{
    if (dirq_purge(*dq) < 0) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Could not purge " << dq->getPath() << " (" << dirq_get_errstr(*dq) << ")"
                                       << fts3::common::commit;
    }
}


void Consumer::purgeAll()
{
    _purge(statusQueue.get());
    _purge(stalledQueue.get());
    _purge(logQueue.get());
    _purge(stagingQueue.get());
    _purge(deletionQueue.get());
}
