/*
 * DeletionStateUpdater.h
 *
 *  Created on: 15 Sep 2014
 *      Author: simonm
 */

#include "StateUpdater.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <vector>

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include "producer_consumer_common.h"
#include "common/logger.h"

using namespace FTS3_COMMON_NAMESPACE;

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
