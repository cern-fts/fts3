/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef DMTASK_H_
#define DMTASK_H_

#include "common/error.h"

#include <boost/any.hpp>

#include <gfal_api.h>

// forward declaration
class JobContext;

class Gfal2Task
{
public:

    /// Default constructor
    Gfal2Task(std::string const & operation) : gfal2_ctx(operation) {}

    /// Copy constructor (deleted)
    Gfal2Task(Gfal2Task const & copy) = delete;

    /// Move constructor
    Gfal2Task(Gfal2Task && copy) : gfal2_ctx(std::move(copy.gfal2_ctx)) {}

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

    /**
     * Sets the proxy-certificate for the given gfal2 task
     *
     * @param ctx : job context containing proxy-certificate information
     */
    void setProxy(JobContext const & ctx);

    /**
     *  Sets the space token
     *
     *  @param spaceToken : space token
     */
    void setSpaceToken(std::string const & spaceToken);


    /**
     * checks if it makes sense to retry the failed job
     *
     * @param errorNo : error code
     * @param category : error category
     * @param message : error message
     */
    bool static doRetry(int errorNo, const std::string& category, const std::string& message);

protected:
    /**
     * gfal2 context wrapper so we can benefit from RAII
     */
    struct Gfal2CtxWrapper
    {
        /// Constructor
        Gfal2CtxWrapper(std::string const & operation) : gfal2_ctx(0), operation(operation)
        {
            // Set up handle
            GError *error = NULL;
            gfal2_ctx = gfal2_context_new(&error);
            if (!gfal2_ctx)
                {
                    std::stringstream ss;
                    ss << operation << " bad initialisation " << error->code << " " << error->message;
                    g_clear_error(&error);
                    // the memory was not allocated so it is safe to throw
                    throw fts3::common::Err_Custom(ss.str());
                }

            if (infosys == "false")
                {
                    gfal2_set_opt_boolean(gfal2_ctx, "BDII", "ENABLED", false, NULL);
                }
            else
                {
                    gfal2_set_opt_string(gfal2_ctx, "BDII", "LCG_GFAL_INFOSYS", (char *) infosys.c_str(), NULL);
                }

            const char *protocols[] = {"rfio", "gsidcap", "dcap", "gsiftp"};

            gfal2_set_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", protocols, 4, &error);
            if (error)
                {
                    std::stringstream ss;
                    ss << operation << " could not set the protocol list " << error->code << " " << error->message;
                    g_clear_error(&error);
                    throw fts3::common::Err_Custom(ss.str());
                }

            gfal2_set_opt_boolean(gfal2_ctx, "GRIDFTP PLUGIN", "SESSION_REUSE", true, &error);
            if (error)
                {
                    std::stringstream ss;
                    ss << operation << " could not set the session reuse " << error->code << " " << error->message;
                    g_clear_error(&error);
                    throw fts3::common::Err_Custom(ss.str());
                }
        }

        /// Copy constructor, steals the pointer from the parameter!
        Gfal2CtxWrapper(Gfal2CtxWrapper && copy) : gfal2_ctx(copy.gfal2_ctx), operation(copy.operation)
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

        /// the gfal2 context itself
        gfal2_context_t gfal2_ctx;
        ///
        std::string const operation;
    };

    /// the infosys used to create all gfal2 contexts
    static std::string infosys;

    /// the operation, e.g. 'DELETION', 'BRINGONLINE', etc.
    Gfal2CtxWrapper gfal2_ctx;
};

#endif /* DMTASK_H_ */
