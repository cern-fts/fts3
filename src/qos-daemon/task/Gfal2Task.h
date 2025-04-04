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

#pragma once
#ifndef GFAL2TASK_H_
#define DMTASK_H_

#include <sstream>
#include <boost/any.hpp>
#include <gfal_api.h>

#include "common/Exceptions.h"

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
     * @param debug: activate debug logging
     */
    static void createPrototype(bool debug = false)
    {
        Gfal2Task::http_log_content = debug;
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
     *  Sets the token
     *
     *  @param token : token
     */
    void setToken(std::string const & token);

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

            if (!gfal2_ctx) {
                std::stringstream ss;
                ss << operation << " bad initialisation " << error->code << " " << error->message;
                g_clear_error(&error);
                // the memory was not allocated so it is safe to throw
                throw fts3::common::UserError(ss.str());
            }

            gfal2_set_opt_boolean(gfal2_ctx, "BDII", "ENABLED", false, NULL);

            // Log HTTP content when debug mode enabled
            if (http_log_content == true) {
                gfal2_set_opt_boolean(gfal2_ctx, "HTTP PLUGIN", "LOG_CONTENT", true, NULL);
            }

            // Make sure "TURL_PROTOCOLS" is configured. If empty, assign default values
            const char* default_turl_protocols[] = {"https", "gsiftp", "root"};
            gsize protocols_len = 0;

            gfal2_get_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", &protocols_len, &error);

            if (error) {
                g_clear_error(&error);
                gfal2_set_opt_string_list(gfal2_ctx, "SRM PLUGIN", "TURL_PROTOCOLS", default_turl_protocols, 3, &error);

                if (error) {
                    std::stringstream ss;
                    ss << operation << " failed to set the TURL protocols default list " << error->code << " "
                       << error->message;
                    g_clear_error(&error);
                    throw fts3::common::UserError(ss.str());
                }
            }

            gfal2_set_opt_boolean(gfal2_ctx, "GRIDFTP PLUGIN", "SESSION_REUSE", true, &error);
            if (error) {
                std::stringstream ss;
                ss << operation << " could not set the session reuse " << error->code << " "
                    << error->message;
                g_clear_error(&error);
                throw fts3::common::UserError(ss.str());
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
        /// the operation, e.g. 'BRINGONLINE', 'ARCHIVING', etc.
        std::string const operation;
    };

    /// Setting to enable logging of HTTP content (when debug mode enabled)
    static bool http_log_content;

    /// the operation, e.g. 'BRINGONLINE', 'ARCHIVING', etc.
    Gfal2CtxWrapper gfal2_ctx;
};

#endif // GFAL2TASK_H_
