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
#ifndef STATEUPDATER_H_
#define STATEUPDATER_H_

#include <glib.h>
#include <string>
#include <vector>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"
#include "config/ServerConfig.h"
#include "msg-bus/producer.h"


/**
 * A base class for carrying out asynchronous state updates,
 * which are accumulated and than send to DB at the same time
 */
class StateUpdater
{

protected:
    // pointer to GenericDbIfce member that updates state
    typedef void (GenericDbIfce::* UpdateStateFunc)(const std::vector<MinFileStatus> &);

public:

    /// Constructor
    StateUpdater(const std::string &operation) :
        db(*db::DBSingleton::instance().getDBObjectInstance()), operation(operation),
        producer(fts3::config::ServerConfig::instance().get<std::string>("MessagingDirectory"))
    {
    }

    /**
     * Functional call for making an asynchronous state update
     *
     * @param job_id : job ID
     * @param file_id : file ID
     * @param state : the new state
     * @param reason : reason for changing the state
     * @param retry : true is the file requires retry, false otherwise
     */
    void operator()(const std::map<std::string, std::map<std::string, std::vector<int> > > &jobs,
        const std::string &state, const std::string &reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        // iterate over jobs
        for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j) {
            std::string const & job_id = it_j->first;
            // iterate over files
            for (auto it_u = it_j->second.begin(); it_u != it_j->second.end(); ++it_u) {
                for (auto it_f = it_u->second.begin(); it_f != it_u->second.end(); ++it_f) {
                    updates.emplace_back(job_id, *it_f, state, reason, retry);
                }
            }
        }
    }

    /**
     *
     */
    void recover()
    {
        std::vector<MinFileStatus> tmp;
        // critical section
        {
            boost::mutex::scoped_lock lock(m);
            tmp.swap(updates);
        }
        recover(tmp);
    }

    /// Destructor
    virtual ~StateUpdater() { }

protected:

    /// this routine is executed in a separate thread
    void runImpl(UpdateStateFunc update_state)
    {
        // temporary vector for DB update
        std::vector<MinFileStatus> tmp;

        while (!boost::this_thread::interruption_requested()) {
            try {
                // wait 10 seconds before checking again
                boost::this_thread::sleep(boost::posix_time::seconds(10));
                // critical section
                {
                    // lock the vector
                    boost::mutex::scoped_lock lock(m);
                    // if the vector is empty there is nothing to do
                    if (updates.empty())
                        continue;
                    // swap the vectors in order to quickly release the lock
                    updates.swap(tmp);
                }

                // run the DB query
                (db.*update_state)(tmp);
            }
            catch (boost::thread_interrupted& ex) {
                FTS3_COMMON_LOGGER_NEWLOG(INFO) << "Requested interruption of " << operation << fts3::common::commit;
                recover(tmp);
                return;
            }
            catch (std::exception& ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << fts3::common::commit;
                recover(tmp);
            }
            catch(...) //use catch-all, the state must be recovered no matter what
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something went really bad, trying to recover!" << fts3::common::commit;
                recover(tmp);
            }
            tmp.clear();
        }
    }

    virtual void recover(const std::vector<MinFileStatus> &recover) = 0;

    /// a vector containing all the updates (to be send to DB)
    std::vector<MinFileStatus> updates;
    /// the mutex guarding the above vector
    boost::mutex m;
    /// DB interface
    GenericDbIfce& db;
    /// operation name ('_delete' or '_staging')
    const std::string operation;

    Producer producer;
};

#endif // STATEUPDATER_H_
