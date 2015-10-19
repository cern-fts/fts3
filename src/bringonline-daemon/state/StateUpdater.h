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

#include <string>
#include <vector>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "db/generic/SingleDbInstance.h"
#include "common/Logger.h"
#include "common/producer_consumer_common.h"

using namespace fts3::common; 

extern bool stopThreads;

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
        db(*db::DBSingleton::instance().getDBObjectInstance()), operation(operation)
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

        while (true) {
            try {
                if (stopThreads)
                    return; //either  gracefully or not
                // wait 10 seconds before checking again
                boost::this_thread::sleep(boost::posix_time::milliseconds(10000));
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
            catch (std::exception& ex) {
                FTS3_COMMON_LOGGER_NEWLOG(ERR)<< ex.what() << commit;
                recover(tmp);
            }
            catch(...) //use catch-all, the state must be recovered no matter what
            {
                FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something went really bad, trying to recover!" << commit;
                recover(tmp);
            }
            tmp.clear();
        }
    }

    /**
     *
     */
    void recover(const std::vector<MinFileStatus> &recover)
    {
        if (!recover.empty()) {
            // lock the vector
            boost::mutex::scoped_lock lock(m);
            // put the items back
            updates.insert(updates.end(), recover.begin(), recover.end());
        }

        message_bringonline msg;
        for (auto itFind = recover.begin(); itFind != recover.end(); ++itFind)
        {
            msg.file_id = itFind->fileId;
            strncpy(msg.job_id, itFind->jobId.c_str(), sizeof(msg.job_id));
            msg.job_id[sizeof(msg.job_id) -1] = '\0';
            strncpy(msg.transfer_status, itFind->state.c_str(), sizeof(msg.transfer_status));
            msg.transfer_status[sizeof(msg.transfer_status) -1] = '\0';
            strncpy(msg.transfer_message, itFind->reason.c_str(), sizeof(msg.transfer_message));
            msg.transfer_message[sizeof(msg.transfer_message) -1] = '\0';

            //store the states into fs to be restored in the next run
            runProducer(msg, operation);
        }
    }

    /// a vector containing all the updates (to be send to DB)
    std::vector<MinFileStatus> updates;
    /// the mutex guarding the above vector
    boost::mutex m;
    /// DB interface
    GenericDbIfce& db;
    /// operation name ('_delete' or '_staging')
    const std::string operation;
};

#endif // STATEUPDATER_H_
