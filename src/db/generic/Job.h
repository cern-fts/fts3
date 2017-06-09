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
#ifndef JOB_H_
#define JOB_H_

#include <string>
#include "OwnedResource.h"


/// Describes the status of a job
/// A job can group several transfers, or several data management operations
/// (i.e. deletions)
class Job: public OwnedResource
{
public:

    Job(): submitTime(0), priority(3), maxTimeInQueue(0),
           jobFinished(0), copyPinLifetime(0), bringOnline(0)
    {
    }

    enum JobType {
        kTypeMultipleReplica = 'R',
        kTypeMultiHop = 'H',
        kTypeSessionReuse = 'Y',
        kTypeRegular = 'N'
    };

    std::string jobId;
    std::string jobState;
    std::string jobParams;
    std::string source;
    std::string destination;
    std::string userDn;
    std::string credId;
    std::string voName;
    std::string reason;
    time_t submitTime;
    int priority;
    std::string submitHost;
    int maxTimeInQueue;
    std::string spaceToken;
    std::string storageClass;
    std::string internalJobParams;
    std::string overwriteFlag;
    time_t jobFinished;
    std::string sourceSpaceToken;
    int copyPinLifetime;
    std::string checksumMode;
    int bringOnline;
    JobType jobType;

    std::string getUserDn() const
    {
        return userDn;
    }
    std::string getVo() const
    {
        return voName;
    }
};

#endif // JOB_H_
