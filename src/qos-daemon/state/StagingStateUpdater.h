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
#ifndef STAGINGSTATEUPDATER_H_
#define STAGINGSTATEUPDATER_H_

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"

#include "StateUpdater.h"

using namespace fts3::common;

/**
 * A utility for carrying out asynchronous state updates,
 * which are accumulated and than send to DB at the same time
 */
class StagingStateUpdater : public StateUpdater
{
public:

    // this is necessary because otherwise the operator would be hidden by the following one
    using StateUpdater::operator();

    /**
     * Updates the file state to STARTED for the given job/file
     *
     * @param urlToIDs : every url with corresponding <jobId, fileId> pair
     * @return true if the update was successful
     */
    bool operator()(std::unordered_multimap< std::string, std::pair<std::string, uint64_t> > urlToIDs)
    {
        if(urlToIDs.empty()) {
            return false;
        }

        std::vector<MinFileStatus> filesState;

        for(auto it: urlToIDs) {
            auto ids = it.second;
            filesState.emplace_back(ids.first, ids.second, "STARTED", "", false);
        }

        try {
            db.updateStagingState(filesState);
        }
        catch (std::exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            return false;
        }
        catch(...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception while updating the bring-online token!" << commit;
            return false;
        }
        return true;
    }

    /**
     * Updates the bring-online token for the given jobs/files
     *
     * @param jobs : jobs with respective files
     * @param token : the token that will be stored in DB
     */
    void operator()(const std::map<std::string, std::map<std::string, std::vector<uint64_t> > > &jobs,
        const std::string &token)
    {
        try {
            db.updateBringOnlineToken(jobs, token);
        }
        catch (std::exception& ex) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
        }
        catch(...) {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception while updating the bring-online token!" << commit;
        }
    }

    /**
     * Updates status per file
     */
    void operator()(const std::string &jobId, uint64_t fileId, const std::string &state, const JobError &error)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.emplace_back(jobId, fileId, state, error.String(), error.IsRecoverable());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "STAGING Update : "
                << fileId << "  " << state << "  " << error.String() << " " << jobId << " " << error.IsRecoverable() << commit;
    }

    void cdmiUpdateFileStateToFinished(const std::string &jobId, uint64_t fileId) {
        db.updateFileStateToQosTerminal(jobId, fileId, "FINISHED");
    }

    void cdmiUpdateFileStateToFailed(const std::string &jobId, uint64_t fileId, const std::string &reason = "") {
        db.updateFileStateToQosTerminal(jobId, fileId, "FAILED", reason);
    }

    void cdmiGetFilesForQosRequestSubmitted(std::vector<QosTransitionOperation> &qosTransitionOps, const std::string& qosOp) {
        db.getFilesForQosTransition(qosTransitionOps, qosOp, true);
    }

    bool cdmiUpdateFileStateToQosRequestSubmitted(const std::string &jobId, uint64_t fileId) {
        return db.updateFileStateToQosRequestSubmitted(jobId, fileId);
    }

    /// Destructor
    virtual ~StagingStateUpdater() {}

    using StateUpdater::recover;

private:
    friend class QoSServer;

    /// Default constructor
    StagingStateUpdater() : StateUpdater("_staging") {}

    /// Copy constructor
    StagingStateUpdater(StagingStateUpdater const &) = delete;
    /// Assignment operator
    StagingStateUpdater & operator=(StagingStateUpdater const &) = delete;

    void run()
    {
        runImpl(&GenericDbIfce::updateStagingState);
    }

    void recover(const std::vector<MinFileStatus> &recover)
    {
        if (!recover.empty()) {
            // lock the vector
            boost::mutex::scoped_lock lock(m);
            // put the items back
            updates.insert(updates.end(), recover.begin(), recover.end());
        }

        fts3::events::MessageBringonline msg;
        for (auto itFind = recover.begin(); itFind != recover.end(); ++itFind)
        {
            msg.set_file_id(itFind->fileId);
            msg.set_job_id(itFind->jobId);
            msg.set_transfer_status(itFind->state);
            msg.set_transfer_message(itFind->reason);

            //store the states into fs to be restored in the next run
            producer.runProducerStaging(msg);
        }
    }
};

#endif // STAGINGSTATEUPDATER_H_
