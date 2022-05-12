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
#include "../QoSServer.h"


/**
 * The StagingContext groups relevant details for a BringOnline batch request.
 * Within the context, files share the same credential ID, storage endpoint and space token.
 *
 * When retrieving staging operations from the database, the above grouping criteria is applied.
 * Files from different jobs may end up in the same batch. More so, multiple file ids may have
 * the same file sURL.
 *
 * As files are grouped into a batch, they are forced on common properties:
 *   - ctx_bringonline_timeout: max(staging_operation.bringonline_timeout)
 *   - ctx_copy_pin_lifetime: max(staging_operation.copy_pin_lifetime)
 *   - ctx_staging_start_time: min(staging_operation.staging_start_time)
 *
 * Timeout is reached when: now() - ctx_staging_start_time > ctx_bringonline_timeout
 *
 * Two data structures are defined internally:
 * 1. map(job_id, map(surl, vector(file_id))).
 *    This data structure is passed to the StateUpdater, responsible for running
 *    a certain update operation across the whole set, on each (job_id, file_id).
 * 2. multimap(surl, pair(job_id, file_id))
 *    Provides an easy way to retrieve a vector(pair(job_id, file_id)) for the given sURL.
 *    This data structure is used predominantly by polling tasks, which operate on a per-file basis.
 */
class StagingContext : public JobContext
{

public:

    using JobContext::add;

    StagingContext(QoSServer &qosServer, const StagingOperation &stagingOp) :
        JobContext(stagingOp.userDn, stagingOp.voName, stagingOp.credId, stagingOp.spaceToken),
        stateUpdater(qosServer.getStagingStateUpdater()), waitingRoom(qosServer.getWaitingRoom()),
        maxPinLifetime(stagingOp.pinLifetime), maxBringonlineTimeout(stagingOp.timeout), minStagingStartTime(time(0)),
        storageEndpoint()
    {
        add(stagingOp);
    }

    StagingContext(const StagingContext &copy) :
        JobContext(copy), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(copy.errorCount),
        maxPinLifetime(copy.maxPinLifetime), maxBringonlineTimeout(copy.maxBringonlineTimeout), minStagingStartTime(copy.minStagingStartTime),
        storageEndpoint(copy.storageEndpoint)
    {}

    StagingContext(StagingContext && copy) :
        JobContext(std::move(copy)), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom), errorCount(std::move(copy.errorCount)),
        maxPinLifetime(copy.maxPinLifetime), maxBringonlineTimeout(copy.maxBringonlineTimeout), minStagingStartTime(copy.minStagingStartTime),
        storageEndpoint(std::move(copy.storageEndpoint))
    {}

    virtual ~StagingContext() {}

    virtual void add(const StagingOperation &stagingOp);

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

    void updateState(const std::string &token)
    {
        stateUpdater(jobs, token);
    }

    int getBringonlineTimeout() const
    {
        return maxBringonlineTimeout;
    }

    int getPinlifetime() const
    {
        return maxPinLifetime;
    }

    std::string getStorageEndpoint() const
    {
        return storageEndpoint;
    }

    time_t getStartTime() const
    {
        return minStagingStartTime;
    }

    bool hasTimeoutExpired();

    std::set<std::string> getSurlsToAbort(const std::set<std::pair<std::string, std::string>>&);

    WaitingRoom<PollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    int incrementErrorCountForSurl(const std::string &surl) {
        return (errorCount[surl] += 1);
    }

protected:
    StagingStateUpdater &stateUpdater;
    WaitingRoom<PollTask> &waitingRoom;
    std::map<std::string, int> errorCount;
    int maxPinLifetime; ///< maximum copy pin lifetime of the batch
    int maxBringonlineTimeout; ///< maximum bringonline timeout of the batch
    time_t minStagingStartTime; ///< first staging start timestamp of the batch
    std::string storageEndpoint; ///< storage endpoint of the batch
};

#endif // STAGINGCONTEXT_H_
