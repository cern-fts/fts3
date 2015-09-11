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

#pragma once
#ifndef FILETRANSFERSTATUS_H_
#define FILETRANSFERSTATUS_H_

#include <string>

/**
 * Describes the status of one file in a transfer job.
 */
class FileTransferStatus
{
public:

    FileTransferStatus() :
             finishTime(time(NULL)), startTime(time(NULL)),
            stagingStart(0), stagingFinished(0),
            numFailures(0), fileId(0), duration(0)
    {
    }

    std::string logicalName;
    std::string sourceSurl;
    std::string destSurl;
    std::string fileState;
    time_t finishTime;
    time_t startTime;
    time_t stagingStart;
    time_t stagingFinished;
    int numFailures;
    std::string reason;
    std::string reasonClass;
    std::string errorScope;
    std::string errorPhase;
    int fileId;
    int fileIndex;
    double duration;
};


/**
 * Retries logging
 */
class FileRetry
{
public:
    int         fileId;
    int         attempt;
    time_t      datetime;
    std::string reason;
};

#endif // FILETRANSFERSTATUS_H_
