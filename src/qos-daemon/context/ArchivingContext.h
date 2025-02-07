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

/**
 * The ArchivingContext groups relevant details for an ArchivingPoll request.
 * Within the context, files share the same credential ID and storage endpoint.
 *
 * Grouping archive monitoring operations is a bit simpler than for staging.
 * As archiving operations are done per file (and not per batch), they only need
 * to share the same credential ID and storage endpoint.
 *
 * Since the archive polling operation is per file, then the timeout for each operation
 * needs to be stored. Timeout is reached when:
 *   now() >= archive_operation_start + archive_operation_timeout
 *
 * The following data structures are defined internally:
 * 1. map(job_id, map(surl, vector(file_id))).
 *    This data structure is passed to the StateUpdater, responsible for running
 *    a certain update operation across the whole set, on each (job_id, file_id).
 * 2. multimap(surl, pair(job_id, file_id))
 *    Provides an easy way to retrieve a vector(pair(job_id, file_id)) for the given sURL.
 *    This data structure is used predominantly by polling tasks, which operate on a per-file basis.
 * 3. map(file_id, expiry_timestamp)
 *    Data structure used to map FileIDs with expiry timestamps (start_time + archive_timeout).
 *    This data structure is used to identify whether the transfer timed out.
 */
class ArchivingContext : public JobContext
{

public:

    using JobContext::add;

    ArchivingContext(QoSServer &qosServer, const ArchivingOperation &archiveOp):
        JobContext(archiveOp.user, archiveOp.voName, archiveOp.credId, ""),
        stateUpdater(qosServer.getArchivingStateUpdater()), waitingRoom(qosServer.getArchivingWaitingRoom())
    {
        add(archiveOp);
        startTime = time(0);
    }

    ArchivingContext(const ArchivingContext &copy) :
        JobContext(copy), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom),
        errorCount(copy.errorCount), expiryMap(copy.expiryMap), startTime(copy.startTime),
        storageEndpoint(copy.storageEndpoint)
    {}

    ArchivingContext(ArchivingContext && copy) :
        JobContext(std::move(copy)), stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom),
        errorCount(std::move(copy.errorCount)), expiryMap(std::move(copy.expiryMap)), startTime(copy.startTime),
        storageEndpoint(std::move(copy.storageEndpoint))
    {}

    virtual ~ArchivingContext() {}

    /**
     * Add archive operation to the context.
     *
     * @param archiveOp : the archive operation to add
     */
    void add(const ArchivingOperation &archiveOp);

    /**
     * Asynchronous update of a single transfer file within a job.
     */
    void updateState(const std::string &jobId, uint64_t fileId, const std::string &state, const JobError &error) const
    {
        stateUpdater(jobId, fileId, state, error);
    }

    /**
     * Asynchronous update across all transfers within the context.
     */
    void updateState(const std::string &state, const JobError &error) const
    {
        stateUpdater(jobs, state, error);
    }

    /**
     * Register current time as the archive start time in the database and
     * add this value to all transfers in the expiry map.
     */
    void setArchiveStartTime()
    {
        stateUpdater(jobs);

        time_t now(time(0));
        for (auto it = expiryMap.begin(); it != expiryMap.end(); it++) {
            expiryMap[it->first] += now;
        }
    }

    /**
     * Checks the expiry map whether the given transfer timed out.
     *
     * @param file_id : transfer file id to check
     * @return true if the transfer timed out, false otherwise
     */
    bool hasTransferTimedOut(uint64_t file_id);

    /**
     * Removes the given transfers from all internal data structures.
     *
     * @param transfers : list of tuples(sURL, job_id, file_id)
     */
    void removeTransfers(const std::list<std::tuple<std::string, std::string, uint64_t>>& timeout_transfers);

    /**
     * Remove a surl and list of IDs from all internal data structures.
     *
     * @param url : the sURL
     * @param ids : vector of pair(job_id, file_id)
     */
    void removeUrlWithIds(const std::string& url, const std::vector<std::pair<std::string, uint64_t>>& ids);

    int incrementErrorCountForSurl(const std::string &surl) {
        return (errorCount[surl] += 1);
    }

    WaitingRoom<ArchivingPollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    inline time_t getStartTime() const
    {
        return startTime;
    }

    std::string getStorageEndpoint() const
    {
        return storageEndpoint;
    }

    // Returns how many file urls belong to this task
    uint64_t getNbUrls() const
    {
        return urlToIDs.size();
    }

private:
    ArchivingStateUpdater &stateUpdater;
    WaitingRoom<ArchivingPollTask> &waitingRoom;
    std::map<std::string, int> errorCount;
    /// Map of FileID --> expire timestamp
    std::map<uint64_t, time_t> expiryMap;
    time_t startTime;
    std::string storageEndpoint;

    friend class ArchivingPollTask;
};

#endif // ARCHIVINGCONTEXT_H_
