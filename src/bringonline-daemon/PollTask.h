/*
 * PollTask.h
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#ifndef POLLTASK_H_
#define POLLTASK_H_

#include "StagingTask.h"

#include "common/definitions.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <set>

#include <algorithm>
#include <iterator>

#include <boost/thread.hpp>

/**
 * A poll task: checks whether a given bring-online operation was successful
 *
 * If the bring-online operation is not finished yet spawns another PollTask.
 * If the operation fails and retries are set spawns another BringOnlineTask.
 *
 * @see StagingTask
 * @see BringOnlineTask
 */
class PollTask : public StagingTask
{

public:

    /**
     * Creates a new PollTask task from another StagingTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    PollTask(StagingTask & copy, std::string token) : StagingTask(copy), token(token), nPolls(0), wait_until()
	{
    	boost::unique_lock<boost::shared_mutex> lock(mx);
    	active_tokens.insert(token);
	}

    /**
     * Copy constructor
     */
    PollTask(PollTask & copy) : StagingTask(copy), token(copy.token), nPolls(copy.nPolls), wait_until(copy.wait_until) {}

    /**
     * Destructor
     */
    virtual ~PollTask()
    {
    	if (gfal2_ctx) cancel(token);
    }

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

    /**
     * @return : true if the task is still waiting, false otherwise
     */
    bool waiting(time_t now)
    {
        return wait_until > now;
    }

    static void cancel(std::string const & token)
    {
    	boost::unique_lock<boost::shared_mutex> lock(mx);
    	active_tokens.erase(token);
    }

    static void cancel(std::set<std::string> const & remove)
    {
    	boost::unique_lock<boost::shared_mutex> lock(mx);
    	// the result of set difference operation
    	std::set<string> result;
    	std::set_difference(
    			active_tokens.begin(), active_tokens.end(),
    			remove.begin(), remove.end(),
    			std::inserter(result, result.end())
    		);
    	// swap the active_token with our result
    	active_tokens.swap(result);
    }

private:
    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    /// set of tokens for ongoing bring-online jobs
    static std::set<std::string> active_tokens;


    /// @return : true if the PollTask is still active, false otherwise
    bool active()
    {
    	boost::shared_lock<boost::shared_mutex> lock(mx);
    	return active_tokens.count(token);
    }

    /// aborts the bring online operation
    void abort();

    /**
     * Gets the interval after next polling should be done
     *
     * @param nPolls : number of polls already done
     */
    static time_t getPollInterval(int nPolls)
    {
        if (nPolls > 5)
            return 180;
        else
            return (2 << nPolls);
    }

    /// the token that will be used for polling
    std::string token;

    /// number of bring online polls
    int nPolls;

    /// wait in the wait room until given time
    time_t wait_until;
};

#endif /* POLLTASK_H_ */
