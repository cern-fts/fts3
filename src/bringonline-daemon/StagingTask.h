/*
 * StagingTask.h
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#ifndef StagingTask_H_
#define StagingTask_H_

#include "StagingStateUpdater.h"

#include "common/definitions.h"
#include "common/logger.h"
#include "common/error.h"

#include "db/generic/SingleDbInstance.h"

#include "cred/DelegCred.h"

#include <string>

#include <boost/tuple/tuple.hpp>
#include <boost/any.hpp>

#include <gfal_api.h>

using namespace FTS3_COMMON_NAMESPACE;

/**
 * An abstract base class for staging tasks: bringing-online and polling
 */
class StagingTask
{

public:

    typedef boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> context_type;

    enum
    {
        vo,
        url,
        job_id,
        file_id,
        copy_pin_lifetime,
        bring_online_timeout,
        dn,
        dlg_id,
        src_space_token
    };

    /**
     * Creates a new StagingTask from a message_bringonline
     *
     * @param ctx : staging task details
     * @param proxy : path to the proxy certificate
     */
    StagingTask(context_type const & ctx) :
        state_update(StagingStateUpdater::instance()), ctx(ctx), gfal2_ctx(), wait_until() {}

    /**
     * Creates a new StagingTask from another StagingTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object!)
     */
    StagingTask(StagingTask & copy) :
        state_update(StagingStateUpdater::instance()), ctx(copy.ctx), gfal2_ctx(copy.gfal2_ctx), wait_until(copy.wait_until) {}

    /**
     * Destructor
     */
    virtual ~StagingTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &) = 0;

    /**
     * Check if the task should be retried
     *
     * @param errorNo : error code
     * @param category : error category
     * @param message : error message
     */
    bool retryTransfer(int errorNo, std::string const & category, std::string const & message);

    /**
     * Creates a prototype for all gfal2 contexts that will be created
     *
     * @infosys : name of the information system
     */
    static void createPrototype(std::string const & infosys)
    {
        StagingTask::infosys = infosys;
    }

    bool waiting(time_t now)
    {
        return wait_until > now;
    }

protected:

    /**
     * gfal2 context wrapper so we can benefit from RAII
     */
    struct Gfal2CtxWrapper
    {
        /// Constructor
        Gfal2CtxWrapper() : gfal2_ctx(0)
        {
            // Set up handle
            GError *error = NULL;
            gfal2_ctx = gfal2_context_new(&error);
            if (!gfal2_ctx)
                {
                    std::stringstream ss;
                    ss << "BRINGONLINE bad initialization " << error->code << " " << error->message;
                    // the memory was not allocated so it is safe to throw
                    throw Err_Custom(ss.str());
                }
        }

        /// Copy constructor, steals the pointer from the parameter!
        Gfal2CtxWrapper(Gfal2CtxWrapper & copy) : gfal2_ctx(copy.gfal2_ctx)
        {
            copy.gfal2_ctx = 0;
        }

        /// conversion to normal gfal2 context
        operator gfal2_context_t()
        {
            return gfal2_ctx;
        }

        /// Destructor
        ~Gfal2CtxWrapper()
        {
            if(gfal2_ctx) gfal2_context_free(gfal2_ctx);
        }

    private:
        /// the gfal2 context itself
        gfal2_context_t gfal2_ctx;
    };

    /// the infosys used to create all gfal2 contexts
    static std::string infosys;

    /// asynchronous state updater
    StagingStateUpdater & state_update;
    /// staging details
    context_type ctx;
    /// gfal2 context
    Gfal2CtxWrapper gfal2_ctx;
    /// wait in the wait room until given time
    time_t wait_until;

};

#endif /* StagingTask_H_ */
