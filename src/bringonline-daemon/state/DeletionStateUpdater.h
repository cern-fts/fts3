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

    static DeletionStateUpdater & instance()
    {
        static DeletionStateUpdater instance;
        return instance;
    }

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
    void operator()(const std::string &jobId, int fileId, const std::string &state, const std::string &reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.emplace_back(jobId, fileId, state, reason, retry);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION Update : " << fileId << "  " << state << "  " << reason << " " << jobId << " " << retry << commit;
    }

private:

    /// Default constructor
    DeletionStateUpdater() : StateUpdater("_delete"), t(run) {}
    /// Copy constructor
    DeletionStateUpdater(DeletionStateUpdater const &) = delete;
    /// Assignment operator
    DeletionStateUpdater & operator=(DeletionStateUpdater const &) = delete;

    /// this rutine is executed in a separate thread
    static void run()
    {
        DeletionStateUpdater & me = instance();
        me.runImpl(&GenericDbIfce::updateDeletionsState);
    }

    /// the worker thread
    boost::thread t;
};

#endif // DELETIONSTATEUPDATER_H_
