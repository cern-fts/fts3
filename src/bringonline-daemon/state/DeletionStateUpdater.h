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
