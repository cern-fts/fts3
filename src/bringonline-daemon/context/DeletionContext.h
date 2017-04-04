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
#ifndef DELETIONCONTEXT_H_
#define DELETIONCONTEXT_H_


#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "cred/DelegCred.h"

#include "JobContext.h"
#include "../BringOnlineServer.h"


class DeletionContext : public JobContext
{
public:

    using JobContext::add;

    /**
     * Constructor (adds the first for deletion)
     *
     * @param ctx : the tuple with data necessary to initialise an instance
     */
    DeletionContext(BringOnlineServer &bringOnlineServer, const DeleteOperation &nsOp):
        JobContext(nsOp.userDn, nsOp.voName, nsOp.credId),
        stateUpdater(bringOnlineServer.getDeletionStateUpdater())
    {
        add(nsOp);
    }

    /**
     * Copy constructor
     *
     * @param copy : other DeletionContext instance
     */
    DeletionContext(const DeletionContext &copy): JobContext(copy), stateUpdater(copy.stateUpdater)
    {}


    /**
     * Move constructor
     *
     * @param copy : the context to be moved
     */
    DeletionContext(DeletionContext && copy): JobContext(std::move(copy)), stateUpdater(copy.stateUpdater)
    {}

    /**
     * Destructor
     */
    virtual ~DeletionContext() {}

    /**
     * Add another file for deletion
     *
     * @param ctx : file for deletion
     */
    void add(const DeleteOperation &nsOp);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void updateState(const std::string &jobId, int fileId, const std::string &state,
        const JobError &error) const;

    /**
     * Bulk state update implementation for srm endpoints
     */
    void updateState(const std::string &state, const JobError &error) const;

private:
    DeletionStateUpdater &stateUpdater;
};

#endif // DELETIONCONTEXT_H_
