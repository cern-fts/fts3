/*
 * BringOnlineTask.h
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#ifndef BRINGONLINETASK_H_
#define BRINGONLINETASK_H_


#include "Gfal2Task.h"
#include "context/StagingContext.h"

#include "common/definitions.h"

#include "db/generic/SingleDbInstance.h"

#include "cred/DelegCred.h"

#include <string>
#include <utility>

#include <boost/scoped_ptr.hpp>
#include <boost/any.hpp>

#include <gfal_api.h>

/**
 * A bring-online task, when started using a thread pool issues a bring online operation
 * if successful spawns a PollTask, if retries are set another BringOnlineTask otherwise
 *
 * @see StagingTask
 */
class BringOnlineTask : public Gfal2Task
{

public:

    /**
     * Creates a new BringOnlineTask from a message_bringonline
     *
     * @param ctx : bring-online task details
     */
    BringOnlineTask(StagingContext const & ctx) : Gfal2Task("BRINGONLINE"), ctx(ctx)
    {
        // set up the space token
        setSpaceToken(ctx.getSpaceToken());
        // set the proxy certificate
        setProxy(ctx);
        // add urls to active
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new BringOnlineTask from another BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    BringOnlineTask(BringOnlineTask && copy) : Gfal2Task(std::move(copy)), ctx(std::move(copy.ctx)) {}

    /**
     * Destructor
     */
    virtual ~BringOnlineTask()
    {
        if (gfal2_ctx) cancel(ctx.getSurls());
    }

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

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

protected:

    /// staging details
    StagingContext ctx;
    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    /// set of tokens (and respective URLs) for ongoing bring-online jobs
    static std::set<std::pair<std::string, std::string>> active_urls;
};


#endif /* BRINGONLINETASK_H_ */
