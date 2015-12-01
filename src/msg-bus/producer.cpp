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

#include <sys/types.h>
#include <errno.h>
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "common/UuidGenerator.h"

#include "producer.h"


static std::string getUniqueTempFileName(const std::string &basedir)
{
    std::string uuidGen = UuidGenerator::generateUUID();
    time_t tmCurrent = time(NULL);
    std::stringstream strmName;
    strmName << basedir << "/" << uuidGen << "_" << tmCurrent;
    return strmName.str();
}


static std::string getNewMessageFile(const std::string &basedir)
{
    return getUniqueTempFileName(basedir);
}


Producer::Producer(const std::string &baseDir): baseDir(baseDir)
{
}


Producer::~Producer()
{
}


static int writeMessage(const void *buffer, size_t bufsize, const std::string &basedir, const std::string &extension)
{
    std::string tempname = getNewMessageFile(basedir);
    if(tempname.length() <= 0)
        return -1;
    // Open
    FILE* fp = NULL;
    fp = fopen(tempname.c_str(), "w");
    if (fp == NULL)
        return errno;

    // Try to write twice
    size_t writeBytes = fwrite(buffer, bufsize, 1, fp);
    if (writeBytes == 0)
        writeBytes = fwrite(buffer, bufsize, 1, fp);

    // Close
    fclose(fp);

    // Rename to final name (sort of commit)
    // Try twice too
    std::string renamedFile = tempname +  extension; //"_ready or _staging or _delete";
    int r = rename(tempname.c_str(), renamedFile.c_str());
    if (r == -1)
        r = rename(tempname.c_str(), renamedFile.c_str());
    if (r == -1)
        return errno;

    return 0;
}


int Producer::runProducerMonitoring(const MessageMonitoring &msg)
{
    return writeMessage(&msg, sizeof(MessageMonitoring), baseDir + "/monitoring", "_ready");
}


int Producer::runProducerStatus(const Message &msg)
{
    return writeMessage(&msg, sizeof(Message), baseDir + "/status", "_ready");
}


int Producer::runProducerStall(const MessageUpdater &msg)
{
    return writeMessage(&msg, sizeof(MessageUpdater), baseDir + "/stalled", "_ready");
}


int Producer::runProducerLog(const MessageLog &msg)
{
    return writeMessage(&msg, sizeof(msg), baseDir + "/logs", "_ready");
}

int Producer::runProducerDeletions(const struct MessageBringonline &msg)
{
    return writeMessage(&msg, sizeof(msg), baseDir + "/status", "_delete");
}


int Producer::runProducerStaging(const struct MessageBringonline &msg)
{
    return writeMessage(&msg, sizeof(msg), baseDir + "/status", "_staging");
}


int Producer::runProducer(const struct MessageBringonline &msg, std::string const & operation)
{
    return writeMessage(&msg, sizeof(MessageMonitoring), baseDir + "/monitoring", "_ready");
}
