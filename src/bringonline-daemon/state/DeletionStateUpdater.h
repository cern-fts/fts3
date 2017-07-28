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
#ifndef DELETIONSTATEUPDATER_H_
#define DELETIONSTATEUPDATER_H_

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
class DeletionStateUpdater : public StateUpdater
{

public:

    virtual ~DeletionStateUpdater() {}

    // make sure it is not hidden by the next one
    using StateUpdater::operator();

    /**
     * Functional call for making an asynchronous state update
     *
     * @param job_id : job ID
     * @param file_id : file ID
     * @param state : the new state
     * @param reason : reason for changing the state
     * @param retry : true is the file requires retry, false otherwise
     */
    void operator()(const std::string &jobId, uint64_t fileId, const std::string &state, const JobError &error)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.emplace_back(jobId, fileId, state, error.String(), error.IsRecoverable());
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION Update : "
                << fileId << "  " << state << "  " << error.String() << " " << jobId << " " << error.IsRecoverable()
                << commit;
    }

    using StateUpdater::recover;

private:
    friend class BringOnlineServer;

    /// Default constructor
    DeletionStateUpdater() : StateUpdater("_delete") {}
    /// Copy constructor
    DeletionStateUpdater(DeletionStateUpdater const &) = delete;
    /// Assignment operator
    DeletionStateUpdater & operator=(DeletionStateUpdater const &) = delete;

    /// this rutine is executed in a separate thread
    void run()
    {
        runImpl(&GenericDbIfce::updateDeletionsState);
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
            producer.runProducerDeletions(msg);
        }
    }
};

#endif // DELETIONSTATEUPDATER_H_
