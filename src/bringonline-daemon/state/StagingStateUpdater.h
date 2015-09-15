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

#ifndef STAGINGSTATEUPDATER_H_
#define STAGINGSTATEUPDATER_H_

#include "StateUpdater.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <vector>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "../../common/Logger.h"
#include "common/producer_consumer_common.h"

using namespace fts3::common; 

extern bool stopThreads;

/**
 * A utility for carrying out asynchronous state updates,
 * which are accumulated and than send to DB at the same time
 */
class StagingStateUpdater : public StateUpdater
{
public:

    /**
     * @return : reference to the singleton instance
     */
    static StagingStateUpdater & instance()
    {
        static StagingStateUpdater instance;
        return instance;
    }

    // this is necessary because otherwise the operator would be hidden by the following one
    using StateUpdater::operator();

    /**
     * Updates the bring-online token for the given jobs/files
     *
     * @param jobs : jobs with respective files
     * @param token : the token that will be stored in DB
     */
    void operator()(std::map< std::string, std::map<std::string, std::vector<int> > > const & jobs, std::string const & token)
    {
        try
            {
                db.updateBringOnlineToken(jobs, token);
            }
        catch(std::exception& ex)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
            }
        catch(...)
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Exception while updating the bring-online token!" << commit;
            }
    }

    /**
     * Updates status per file
     */
    void operator()(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.push_back(value_type(file_id, state, reason, job_id, retry));
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "STAGING Update : " << file_id << "  " << state << "  " << reason << " " << job_id << " " << retry << commit;
    }

    /// Destructor
    virtual ~StagingStateUpdater() {}

private:

    /// Default constructor
    StagingStateUpdater() : StateUpdater("_staging"), t(run) {}
    /// Copy constructor (not implemented)
    StagingStateUpdater(StagingStateUpdater const &);
    /// Assignment operator (not implemented)
    StagingStateUpdater & operator=(StagingStateUpdater const &);

    /// this rutine is executed in a separate thread
    static void run()
    {
        StagingStateUpdater & me = instance();
        me.run_impl(&GenericDbIfce::updateStagingState);
    }

    /// the worker thread
    boost::thread t;
};

#endif /* STAGINGSTATEUPDATER_H_ */
