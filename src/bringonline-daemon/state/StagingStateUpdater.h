/*
 * StagingStateUpdater.h
 *
 *  Created on: 30 Jun 2014
 *      Author: simonm
 */

#ifndef STAGINGSTATEUPDATER_H_
#define STAGINGSTATEUPDATER_H_

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
    void operator()(std::map< std::string, std::vector<std::pair<int, std::string> > > const & jobs, std::string const & token)
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
