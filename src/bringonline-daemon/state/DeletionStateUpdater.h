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

#ifndef DELETIONSTATEUPDATER_H_
#define DELETIONSTATEUPDATER_H_

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
    void operator()(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.push_back(value_type(file_id, state, reason, job_id, retry));
        FTS3_COMMON_LOGGER_NEWLOG(INFO) << "DELETION Update : " << file_id << "  " << state << "  " << reason << " " << job_id << " " << retry << commit;
    }

private:

    /// Default constructor
    DeletionStateUpdater() : StateUpdater("_delete"), t(run) {}
    /// Copy constructor (not implemented)
    DeletionStateUpdater(DeletionStateUpdater const &);
    /// Assignment operator (not implemented)
    DeletionStateUpdater & operator=(DeletionStateUpdater const &);

    /// this rutine is executed in a separate thread
    static void run()
    {
        DeletionStateUpdater & me = instance();
        me.run_impl(&GenericDbIfce::updateDeletionsState);
    }

    /// the worker thread
    boost::thread t;
};

#endif /* DELETIONSTATEUPDATER_H_ */
