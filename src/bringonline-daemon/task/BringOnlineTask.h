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
    virtual ~BringOnlineTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

protected:
    /// staging details
    StagingContext ctx;
};


#endif /* BRINGONLINETASK_H_ */
