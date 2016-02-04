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

#include "common/Logger.h"


Producer::Producer(const std::string &baseDir): baseDir(baseDir),
    monitoringQueue(baseDir + "/monitoring"), statusQueue(baseDir + "/status"),
    stalledQueue(baseDir + "/stalled"), logQueue(baseDir + "/logs"),
    deletionQueue(baseDir + "/deletion"), stagingQueue(baseDir + "/staging")
{
}


Producer::~Producer()
{
}


static int writeMessage(DirQ &dirqHandle, const google::protobuf::Message &msg)
{
    char tempTemplate[PATH_MAX];
    snprintf(tempTemplate, PATH_MAX, "%s/%%%%%%%%%%%%%%%%", dirqHandle.getPath().c_str());

    boost::filesystem::path temp = boost::filesystem::unique_path(tempTemplate);
    const std::string tempPath = temp.native();

    try {
        std::ofstream stream(tempPath);
        msg.SerializeToOstream(&stream);
    }
    catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)
            << "Could not write message to " << tempPath << " (" << ex.what() << ")"
            << fts3::common::commit;
        return EIO;
    }


    if (dirq_add_path(dirqHandle, tempPath.c_str()) == NULL) {
        return dirq_get_errcode(dirqHandle);
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
    char tempTemplate[PATH_MAX];
    snprintf(tempTemplate, PATH_MAX, "%s/%%%%%%%%%%%%%%%%", monitoringQueue.getPath().c_str());

    boost::filesystem::path temp = boost::filesystem::unique_path(tempTemplate);
    const std::string tempPath = temp.native();

    try {
        std::ofstream stream(tempPath);
        stream << serialized;
    }
    catch (const std::exception &ex) {
        FTS3_COMMON_LOGGER_NEWLOG(ERR)
        << "Could not write message to " << tempPath << " (" << ex.what() << ")"
        << fts3::common::commit;
        return EIO;
    }


    if (dirq_add_path(monitoringQueue, tempPath.c_str()) == NULL) {
        return dirq_get_errcode(monitoringQueue);
    }

    return 0;
}
