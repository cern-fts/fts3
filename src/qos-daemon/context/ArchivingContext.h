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
#ifndef ARCHIVINGCONTEXT_H_
#define ARCHIVINGCONTEXT_H_


#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cred/DelegCred.h"
#include "db/generic/ArchivingOperation.h"

#include "JobContext.h"
#include "../QoSServer.h"


class ArchivingContext : public JobContext
{

public:

    using JobContext::add;

    ArchivingContext(QoSServer &qosServer, const ArchivingOperation &archiveOp):
        JobContext(archiveOp.user, archiveOp.voName, archiveOp.credId, ""),
        stateUpdater(qosServer.getArchivingStateUpdater()), waitingRoom(qosServer.getArchivingWaitingRoom()),
		archiveTimeout(archiveOp.timeout)
    {
        add(archiveOp);
        startTime = time(0);
    }

    ArchivingContext(const ArchivingContext &copy) :
        JobContext(copy), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(copy.errorCount),
		archiveTimeout(copy.archiveTimeout), startTime(copy.startTime)
    {}

    ArchivingContext(ArchivingContext && copy) :
        JobContext(std::move(copy)), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(std::move(copy.errorCount)),
		archiveTimeout(copy.archiveTimeout), startTime(copy.startTime)
    {}

    virtual ~ArchivingContext() {}

    void add(const ArchivingOperation &stagingOp);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void updateState(const std::string &jobId, uint64_t fileId, const std::string &state, const JobError &error) const
    {
        stateUpdater(jobId, fileId, state, error);
    }

    void updateState(const std::string &state, const JobError &error) const
    {
        stateUpdater(jobs, state, error);
    }

    void setStartTime()
    {
         stateUpdater(jobs);
    }

    int getArchiveTimeout() const
    {
        return archiveTimeout;
    }



    bool hasTimeoutExpired();

    std::set<std::string> getSurlsToAbort(const std::set<std::pair<std::string, std::string>>&);

    WaitingRoom<ArchivingPollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    int incrementErrorCountForSurl(const std::string &surl) {
        return (errorCount[surl] += 1);
    }

private:
    ArchivingStateUpdater &stateUpdater;
    WaitingRoom<ArchivingPollTask> &waitingRoom;
    std::map<std::string, int> errorCount;
    int archiveTimeout;
    time_t startTime;
};

#endif // ARCHIVINGCONTEXT_H_
