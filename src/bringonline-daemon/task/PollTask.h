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
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new PollTask task from a BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    PollTask(BringOnlineTask && copy, std::string const & token) : BringOnlineTask(std::move(copy)), token(token), nPolls(0), wait_until()
    {
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
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
        if (gfal2_ctx) cancel(ctx.getSurls());
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

    static void cancel(std::set<std::pair<std::string, std::string> > const & urls)
    {
        if (urls.empty()) return;

        boost::unique_lock<boost::shared_mutex> lock(mx);
        auto begin = active_urls.lower_bound(*urls.begin());
        auto end   = active_urls.upper_bound(*urls.rbegin());
        for (auto it = begin; it != end;)
            if (urls.count(*it))
                active_urls.erase(it++);
            else
                ++it;
    }

private:
    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    /// set of tokens (and respective URLs) for ongoing bring-online jobs
    static std::set<std::pair<std::string, std::string>> active_urls;

    /// checks if the bring online task was cancelled and removes those URLs that were from the context
    void handle_canceled();

    /// checks if the bring online task timed-out and removes respective URLs from the context
    void handle_timeout();

    /// aborts the bring online operation for the given URLs
    void abort(std::vector<char const *> const & urls);

    /**
     * Gets the interval after next polling should be done
     *
     * @param nPolls : number of polls already done
     */
    static time_t getPollInterval(int nPolls)
    {
        if (nPolls > 9)
            return 600;
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
