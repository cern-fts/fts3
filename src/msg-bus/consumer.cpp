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
#include "consumer.h"


struct sort_functor_updater
{
    bool operator()(const MessageUpdater & a, const MessageUpdater & b) const
    {
        return a.timestamp < b.timestamp;
    }
};

struct sort_functor_status
{
    bool operator()(const Message & a, const Message & b) const
    {
        return a.timestamp < b.timestamp;
    }
};


Consumer::Consumer(const std::string &baseDir, unsigned limit):
    baseDir(baseDir), limit(limit)
{
    monitoringQueue = dirq_new((baseDir + "/monitoring").c_str());
    statusQueue = dirq_new((baseDir + "/status").c_str());
    stalledQueue = dirq_new((baseDir + "/stalled").c_str());
    logQueue = dirq_new((baseDir + "/logs").c_str());
    deletionQueue = dirq_new((baseDir + "/deletion").c_str());
    stagingQueue = dirq_new((baseDir + "/staging").c_str());
}


Consumer::~Consumer()
{
    dirq_free(monitoringQueue);
    dirq_free(statusQueue);
    dirq_free(stalledQueue);
    dirq_free(logQueue);
    dirq_free(deletionQueue);
    dirq_free(stagingQueue);
}


template <typename MSG>
static int genericConsumer(dirq_t dirq, std::vector<MSG> &messages)
{
    MSG buffer;

    for (auto iter = dirq_first(dirq); iter != NULL; iter = dirq_next(dirq)) {
        if (dirq_lock(dirq, iter, 0) == 0) {
            const char *path = dirq_get_path(dirq, iter);

            int fd = open(path, O_RDONLY);
            if (fd < 0) {
                buffer.set_error(errno);
            }
            else if (read(fd, &buffer, sizeof(buffer)) != sizeof(buffer)) {
                buffer.set_error(EBADMSG);
            }
            messages.emplace_back(buffer);

            close(fd);

            dirq_remove(dirq, iter);
        }
    }

    return 0;
}


int Consumer::runConsumerMonitoring(std::vector<struct MessageMonitoring> &messages)
{
    return genericConsumer<MessageMonitoring>(monitoringQueue, messages);
}


int Consumer::runConsumerStatus(std::vector<struct Message> &messages)
{
    return genericConsumer<Message>(statusQueue, messages);
}


int Consumer::runConsumerStall(std::vector<struct MessageUpdater> &messages)
{
    return genericConsumer<MessageUpdater>(stalledQueue, messages);
}


int Consumer::runConsumerLog(std::map<int, struct MessageLog> &messages)
{
    MessageLog buffer;

    for (auto iter = dirq_first(logQueue); iter != NULL; iter = dirq_next(logQueue)) {
        if (dirq_lock(logQueue, iter, 0) == 0) {
            const char *path = dirq_get_path(logQueue, iter);

            int fd = open(path, O_RDONLY);
            if (fd < 0) {
                continue;
            }

            if (read(fd, &buffer, sizeof(buffer)) == sizeof(buffer)) {
                messages[buffer.file_id] = buffer;
            }

            close(fd);

            dirq_remove(logQueue, iter);
        }
    }

    return 0;
}


int Consumer::runConsumerDeletions(std::vector<struct MessageBringonline> &messages)
{
    return genericConsumer<MessageBringonline>(deletionQueue, messages);
}


int Consumer::runConsumerStaging(std::vector<struct MessageBringonline> &messages)
{
    return genericConsumer<MessageBringonline>(stagingQueue, messages);
}
