/*
 * StagingStateUpdater.h
 *
 *  Created on: 30 Jun 2014
 *      Author: simonm
 */

#ifndef STAGINGSTATEUPDATER_H_
#define STAGINGSTATEUPDATER_H_

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <vector>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include "producer_consumer_common.h"
#include "common/logger.h"

using namespace FTS3_COMMON_NAMESPACE;

extern bool stopThreads;

/**
 * A utility for carrying out asynchronous state updates,
 * which are accumulated and than send to DB at the same time
 */
class StagingStateUpdater
{
public:

    /**
     * @retrun : reference to the singleton instance
     */
    static StagingStateUpdater & instance()
    {
        static StagingStateUpdater instance;
        return instance;
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
    void operator()(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry)
    {
        // lock the vector
        boost::mutex::scoped_lock lock(m);
        updates.push_back(value_type(file_id, state, reason, job_id, retry));
    }

    void recover()
    {
        std::vector<value_type> tmp;
        // critical section
        {
            boost::mutex::scoped_lock lock(m);
            tmp.swap(updates);
        }

        recover(tmp);
    }

    /// Destructor
    virtual ~StagingStateUpdater() {}

private:
    // typedef for convenience
    typedef boost::tuple<int, std::string, std::string, std::string, bool> value_type;

    /// Default constructor
    StagingStateUpdater() : t(run), db(*db::DBSingleton::instance().getDBObjectInstance()) {}
    /// Copy constructor (not implemented)
    StagingStateUpdater(StagingStateUpdater const &);
    /// Assignment operator (not implemented)
    StagingStateUpdater & operator=(StagingStateUpdater const &);

    /// this rutine is executed in a separate thread
    static void run()
    {
        StagingStateUpdater & me = instance();
        // temporary vector for DB update
        std::vector<value_type> tmp;

        while (true)
            {
                try
                    {
                        if(stopThreads) return;//either  gracefully or not
                        // wait 10 seconds before checking again
                        boost::this_thread::sleep(boost::posix_time::milliseconds(10000));
                        // critical section
                        {
                            // lock the vector
                            boost::mutex::scoped_lock lock(me.m);
                            // if the vector is empty there is nothing to do
                            if (me.updates.empty()) continue;
                            // swap the vectors in order to quickly release the lock
                            me.updates.swap(tmp);
                        }

                        // run the DB query
                        me.db.updateStagingState(tmp);
                    }
                catch(std::exception& ex)
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << ex.what() << commit;
                        me.recover(tmp);
                    }
                catch(...) //use catch-all, the state must be recovered no matter what
                    {
                        FTS3_COMMON_LOGGER_NEWLOG(ERR) << "Something went really bad, trying to recover!" << commit;
                        me.recover(tmp);
                    }
                tmp.clear();
            }
    }

    void recover(std::vector<value_type> const & recover)
    {
        message_bringonline msg;
        std::vector<value_type>::const_iterator itFind;
        for (itFind = recover.begin(); itFind != recover.end(); ++itFind)
            {
                value_type const & tupleRecord = *itFind;
                int file_id = boost::get<0>(tupleRecord);
                std::string const & transfer_status = boost::get<1>(tupleRecord);
                std::string const & transfer_message = boost::get<2>(tupleRecord);
                std::string const & job_id = boost::get<3>(tupleRecord);

                msg.file_id = file_id;
                strncpy(msg.job_id, job_id.c_str(), sizeof(msg.job_id));
                msg.job_id[sizeof(msg.job_id) -1] = '\0';
                strncpy(msg.transfer_status, transfer_status.c_str(), sizeof(msg.transfer_status));
                msg.transfer_status[sizeof(msg.transfer_status) -1] = '\0';
                strncpy(msg.transfer_message, transfer_message.c_str(), sizeof(msg.transfer_message));
                msg.transfer_message[sizeof(msg.transfer_message) -1] = '\0';

                //store the states into fs to be restored in the next run
                runProducerStaging(msg);
            }
    }

    /// the worker thread
    boost::thread t;
    /// a vector containing all the updates (to be send to DB)
    std::vector<value_type> updates;
    /// the mutex guarding the above vector
    boost::mutex m;
    /// DB interface
    GenericDbIfce& db;
};

#endif /* STAGINGSTATEUPDATER_H_ */
