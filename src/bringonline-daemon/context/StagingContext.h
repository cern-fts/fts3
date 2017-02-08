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
#ifndef STAGINGCONTEXT_H_
#define STAGINGCONTEXT_H_


#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cred/DelegCred.h"

#include "JobContext.h"
#include "../BringOnlineServer.h"


class StagingContext : public JobContext
{

public:

    using JobContext::add;

    StagingContext(BringOnlineServer &bringOnlineServer, const StagingOperation &stagingOp):
        JobContext(stagingOp.userDn, stagingOp.voName, stagingOp.credId, stagingOp.spaceToken),
        stateUpdater(bringOnlineServer.getStagingStateUpdater()), waitingRoom(bringOnlineServer.getWaitingRoom()),
        pinLifetime(stagingOp.pinLifetime), bringonlineTimeout(stagingOp.timeout)
    {
        add(stagingOp);
        startTime = time(0);
    }

    StagingContext(const StagingContext &copy) :
        JobContext(copy), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(copy.errorCount),
        pinLifetime(copy.pinLifetime), bringonlineTimeout(copy.bringonlineTimeout), startTime(copy.startTime)
    {}

    StagingContext(StagingContext && copy) :
        JobContext(std::move(copy)), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(std::move(copy.errorCount)),
        pinLifetime(copy.pinLifetime), bringonlineTimeout(copy.bringonlineTimeout), startTime(copy.startTime)
    {}

    virtual ~StagingContext() {}

    void add(const StagingOperation &stagingOp);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void updateState(const std::string &jobId, int fileId, const std::string &state, const std::string &reason, bool retry) const
    {
        stateUpdater(jobId, fileId, state, reason, retry);
    }

    void updateState(const std::string &state, const std::string &reason, bool retry) const
    {
        stateUpdater(jobs, state, reason, retry);
    }

    void updateState(const std::string &token)
    {
        stateUpdater(jobs, token);
    }

    int getBringonlineTimeout() const
    {
        return bringonlineTimeout;
    }

    int getPinlifetime() const
    {
        return pinLifetime;
    }

    bool hasTimeoutExpired();

    std::set<std::string> getSurlsToAbort(const std::set<std::pair<std::string, std::string>>&);

    WaitingRoom<PollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    int incrementErrorCountForSurl(const std::string &surl) {
        return (errorCount[surl] += 1);
    }

private:
    StagingStateUpdater &stateUpdater;
    WaitingRoom<PollTask> &waitingRoom;
    std::map<std::string, int> errorCount;
    int pinLifetime;
    int bringonlineTimeout;
    time_t startTime;
};

#endif // STAGINGCONTEXT_H_
