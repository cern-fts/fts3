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
#include <boost/thread/tss.hpp>
#include "DirQ.h"

#include "common/Logger.h"


Producer::Producer(const std::string &baseDir): baseDir(baseDir),
    monitoringQueue(new DirQ(baseDir + "/monitoring")), statusQueue(new DirQ(baseDir + "/status")),
    stalledQueue(new DirQ(baseDir + "/stalled")), logQueue(new DirQ(baseDir + "/logs")),
    deletionQueue(new DirQ(baseDir + "/deletion")), stagingQueue(new DirQ(baseDir + "/staging"))
{
}


Producer::~Producer()
{
}


boost::thread_specific_ptr<std::istringstream> msgBuffer;


static void populateBuffer(const std::string &msg)
{
    if (msgBuffer.get() == NULL) {
        msgBuffer.reset(new std::istringstream());
    }
    msgBuffer->clear();
    msgBuffer->str(msg);
}


static int producerDirqW(dirq_t, char *buffer, size_t length)
{
    return msgBuffer->readsome(buffer, length);
}


static int writeMessage(std::unique_ptr<DirQ> &dirqHandle, const google::protobuf::Message &msg)
{
    populateBuffer(msg.SerializeAsString());
    if (dirq_add(*dirqHandle, producerDirqW) == NULL) {
        return dirq_get_errcode(*dirqHandle);
    }

    return 0;
}


int Producer::runProducerStatus(const fts3::events::Message &msg)
{
    return writeMessage(statusQueue, msg);
}


int Producer::runProducerStall(const fts3::events::MessageUpdater &msg)
{
    return writeMessage(stalledQueue, msg);
}


int Producer::runProducerLog(const fts3::events::MessageLog &msg)
{
    return writeMessage(logQueue, msg);
}

int Producer::runProducerDeletions(const fts3::events::MessageBringonline &msg)
{
    return writeMessage(deletionQueue, msg);
}


int Producer::runProducerStaging(const fts3::events::MessageBringonline &msg)
{
    return writeMessage(stagingQueue, msg);
}


int Producer::runProducerMonitoring(const std::string &serialized)
{
    populateBuffer(serialized);
    if (dirq_add(*monitoringQueue, producerDirqW) == NULL) {
        return dirq_get_errcode(*monitoringQueue);
    }

    return 0;
}
