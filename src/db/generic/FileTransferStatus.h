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

#include <iostream>

/**
 * Describes the status of one file in a transfer job.
 */
class FileTransferStatus
{
public:

    FileTransferStatus() :
        numFailures(0), finish_time(time(NULL)), start_time(time(NULL)),
        staging_start(0), staging_finished(0),
        fileId(0), duration(0)
    {
    }

    ~FileTransferStatus()
    {
    }

    std::string logicalName;
    std::string sourceSURL;
    std::string destSURL;
    std::string transferFileState;
    int numFailures;
    std::string reason;
    std::string reason_class;
    time_t finish_time;
    time_t start_time;
    time_t staging_start;
    time_t staging_finished;
    std::string error_scope;
    std::string error_phase;
    int fileId;
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
