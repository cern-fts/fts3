/*
 * Gfal2Task.h
 *
 *  Created on: 3 Jul 2014
 *      Author: simonm
 */

#ifndef DMTASK_H_
#define DMTASK_H_

#include "common/error.h"

#include <boost/any.hpp>

#include <gfal_api.h>

using namespace FTS3_COMMON_NAMESPACE;

class Gfal2Task
{
public:

    /// Default constructor
    Gfal2Task() : gfal2_ctx() {}

    /// Copy constructor
    Gfal2Task(Gfal2Task & copy) : gfal2_ctx(copy.gfal2_ctx) {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &) = 0;

    /// Destructor
    virtual ~Gfal2Task() {}

    /**
     * Creates a prototype for all gfal2 contexts that will be created
     *
     * @infosys : name of the information system
     */
    static void createPrototype(std::string const & infosys)
    {
        Gfal2Task::infosys = infosys;
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

            if (infosys == "false")
                {
                    gfal2_set_opt_boolean(gfal2_ctx, "BDII", "ENABLED", false, NULL);
                }
            else
                {
                    gfal2_set_opt_string(gfal2_ctx, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
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

    /// gfal2 context
    Gfal2CtxWrapper gfal2_ctx;
};

#endif /* DMTASK_H_ */
