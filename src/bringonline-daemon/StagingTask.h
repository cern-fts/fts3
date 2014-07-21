/*
 * StagingTask.h
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#ifndef StagingTask_H_
#define StagingTask_H_

#include "Gfal2Task.h"
#include "StagingStateUpdater.h"
#include "StagingContext.h"

#include "common/definitions.h"
#include "common/logger.h"
#include "common/error.h"

#include "db/generic/SingleDbInstance.h"

#include "cred/DelegCred.h"

#include <string>
#include <utility>

#include <boost/tuple/tuple.hpp>
#include <boost/any.hpp>

/**
 * An abstract base class for staging tasks: bringing-online and polling
 */
class StagingTask : public Gfal2Task
{

public:

    /**
     * Creates a new StagingTask from a message_bringonline
     *
     * @param ctx : staging task details
     */
    StagingTask(StagingContext const & ctx) :
        Gfal2Task(), state_update(StagingStateUpdater::instance()), ctx(ctx)
    {
        // set up the gfal2 context
        GError *error = NULL;

        if (!ctx.srcSpaceToken.empty())
            {
                gfal2_set_opt_string(gfal2_ctx, "SRM PLUGIN", "SPACETOKENDESC", (char *) ctx.srcSpaceToken.c_str(), &error);
                if (error)
                    {
                        std::stringstream ss;
                        ss << "BRINGONLINE Could not set the space token " << error->code << " " << error->message;
                        throw Err_Custom(ss.str());
                    }
            }

        // set the proxy certificate
        setProxy();
    }

    /**
     * Creates a new StagingTask from another StagingTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object!)
     */
    StagingTask(StagingTask & copy) :
        Gfal2Task(copy), state_update(StagingStateUpdater::instance()), ctx(copy.ctx) {}

    /**
     * Destructor
     */
    virtual ~StagingTask() {}

    /**
     * Check if the task should be retried
     *
     * @param errorNo : error code
     * @param category : error category
     * @param message : error message
     */
    static bool retryTransfer(int errorNo, std::string const & category, std::string const & message);

protected:

    /**
     * sets the proxy
     */
    void setProxy();

    /// asynchronous state updater
    StagingStateUpdater & state_update;
    /// staging details
    StagingContext const ctx;

};

#endif /* StagingTask_H_ */
