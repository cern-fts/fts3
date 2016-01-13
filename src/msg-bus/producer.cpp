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

#include "producer.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <glib.h>


Producer::Producer(const std::string &baseDir): baseDir(baseDir),
    monitoringQueue(baseDir + "/monitoring"), statusQueue(baseDir + "/status"),
    stalledQueue(baseDir + "/stalled"), logQueue(baseDir + "/logs"),
    deletionQueue(baseDir + "/deletion"), stagingQueue(baseDir + "/staging")
{
}


Producer::~Producer()
{
}


int Producer::writeMessage(DirQ &dirqHandle, const void *buffer, size_t bufsize)
{
    char tempName[PATH_MAX];
    snprintf(tempName, PATH_MAX, "%s/XXXXXX", baseDir.c_str());

    int tempFd = mkstemp(tempName);
    if (tempFd < 0) {
        return errno;
    }

    ssize_t ret = write(tempFd, buffer, bufsize);
    close(tempFd);
    if (ret != static_cast<ssize_t>(bufsize)) {
        return EBADMSG;
    }

    if (dirq_add_path(dirqHandle, tempName) == NULL) {
        return dirq_get_errcode(dirqHandle);
    }
    return 0;
}


int Producer::runProducerMonitoring(const MessageMonitoring &msg)
{
    return writeMessage(monitoringQueue, &msg, sizeof(MessageMonitoring));
}


int Producer::runProducerStatus(const Message &msg)
{
    return writeMessage(statusQueue, &msg, sizeof(Message));
}


int Producer::runProducerStall(const MessageUpdater &msg)
{
    return writeMessage(stalledQueue, &msg, sizeof(MessageUpdater));
}


int Producer::runProducerLog(const MessageLog &msg)
{
    return writeMessage(logQueue, &msg, sizeof(msg));
}

int Producer::runProducerDeletions(const struct MessageBringonline &msg)
{
    return writeMessage(deletionQueue, &msg, sizeof(msg));
}


int Producer::runProducerStaging(const struct MessageBringonline &msg)
{
    return writeMessage(stagingQueue, &msg, sizeof(msg));
}
