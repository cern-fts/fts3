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
     * Updates the bring-online token for the given jobs/files
     *
     * @param jobs : jobs with respective files
     * @param token : the token that will be stored in DB
     */
    void operator()(const std::map<std::string, std::map<std::string, std::vector<int> > > &jobs,
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
    void operator()(const std::string &jobId, int fileId, const std::string &state, const std::string &reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.emplace_back(jobId, fileId, state, reason, retry);
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "STAGING Update : " << fileId << "  " << state << "  " << reason << " " << jobId << " " << retry << commit;
    }

    /// Destructor
    virtual ~StagingStateUpdater() {}

private:
    friend class BringOnlineServer;

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
};

#endif // STAGINGSTATEUPDATER_H_
