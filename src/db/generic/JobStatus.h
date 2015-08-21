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
#include <ctime>

class JobStatus
{

public:
    JobStatus():fileIndex(0), submitTime(0), numFiles(0), priority(3) {}
    ~JobStatus() {}

    /**
     * The ID of the job the information refers to.
     */
    std::string jobID;

    /**
     * Current status of the job.
     */
    std::string jobStatus;

    /**
     * The name of the channel the job was assigned to (null if no channel assigned).
     */
    std::string fileStatus;

    /**
     * file index
     */
    int fileIndex;

    /**
     * Distinguished Name (DN) of client owning job.
     */
    std::string clientDN;

    /**
     * Small reason reporting problems while processing the job.
     */
    std::string reason;

    /**
     * Name of VO owning job.
     */
    std::string voName;

    /**
     * Time of submission.
     */
    std::time_t submitTime;

    /**
     * Number of files (transfers) that are part of the job.
     */
    long long numFiles;

    /**
     * Job priority on the server.
     */
    int priority;
};
