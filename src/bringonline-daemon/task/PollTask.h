/*
 * PollTask.h
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#ifndef POLLTASK_H_
#define POLLTASK_H_

#include "BringOnlineTask.h"

#include "common/definitions.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <set>

#include <algorithm>
#include <iterator>

#include <unordered_map>
#include <set>

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
class PollTask : public BringOnlineTask
{

public:
    /**
     * Creates a PollTask from StagingContext (for recovery purposes only)
     *
     * @param ctx : staging context (recover from DB after crash)
     * @param token : token that is needed for polling
     */
    PollTask(StagingContext const & ctx, std::string const & token) : BringOnlineTask(ctx), token(token), nPolls(0), wait_until(0)
    {
        boost::unique_lock<boost::shared_mutex> lock(mx);
        std::set<std::string> surls = ctx.getSurls();
        active_tokens[token].insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new PollTask task from a BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    PollTask(BringOnlineTask && copy, std::string const & token) : BringOnlineTask(std::move(copy)), token(token), nPolls(0), wait_until()
    {
        boost::unique_lock<boost::shared_mutex> lock(mx);
        std::set<std::string> surls = ctx.getSurls();
        active_tokens[token].insert(surls.begin(), surls.end());
    }

    /**
     * Move constructor
     */
    PollTask(PollTask && copy) : BringOnlineTask(std::move(copy)), token(copy.token), nPolls(copy.nPolls), wait_until(copy.wait_until) {}

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

    static void cancel(std::unordered_map< std::string, std::set<std::string> > const & remove);

private:
    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    /// set of tokens (and respective URLs) for ongoing bring-online jobs
    static std::unordered_map< std::string, std::set<std::string> > active_tokens;

    /// aborts the bring online operation
    bool abort();

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
