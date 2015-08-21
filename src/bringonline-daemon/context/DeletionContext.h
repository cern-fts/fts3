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

#ifndef DELETIONCONTEXT_H_
#define DELETIONCONTEXT_H_

#include "JobContext.h"
#include "state/DeletionStateUpdater.h"

#include "cred/DelegCred.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

#include <boost/tuple/tuple.hpp>

class DeletionContext : public JobContext
{

public:

    using JobContext::add;

    // typedef for convenience
    // vo_name, source_url, job_id, file_id, user_dn, cred_id
    typedef boost::tuple<std::string, std::string, std::string, int, std::string, std::string> context_type;

    enum {vo_name, source_url, job_id, file_id, user_dn, cred_id};

    /**
     * Constructor (adds the first for deletion)
     *
     * @param ctx : the tuple with data necessary to initialise an instance
     */
    DeletionContext(context_type const & ctx) :
        JobContext(boost::get<user_dn>(ctx), boost::get<vo_name>(ctx), boost::get<cred_id>(ctx))
    {
        add(ctx);
    }

    /**
     * Copy constructor
     *
     * @param copy : other DeletionContext instance
     */
    DeletionContext(DeletionContext const & copy) : JobContext(copy) {}


    /**
     * Move constructor
     *
     * @param copy : the context to be moved
     */
    DeletionContext(DeletionContext && copy) :
        JobContext(std::move(copy)) {}

    /**
     * Destructor
     */
    virtual ~DeletionContext() {}

    /**
     * Add another file for deletion
     *
     * @param ctx : file for deletion
     */
    void add(context_type const & ctx);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void state_update(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry) const
    {
        static DeletionStateUpdater & state_update = DeletionStateUpdater::instance();
        state_update(job_id, file_id, state, reason, retry);
    }

    /**
     * Bulk state update implementation for srm endpoints
     */
    void state_update(std::string const & state, std::string const & reason, bool retry) const
    {
        static DeletionStateUpdater & state_update = DeletionStateUpdater::instance();
        state_update(jobs, state, reason, retry);
    }
};

#endif /* DELETIONCONTEXT_H_ */
