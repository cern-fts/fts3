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

#ifndef DELETIONTASK_H_
#define DELETIONTASK_H_


#include "Gfal2Task.h"

#include "context/DeletionContext.h"

#include "common/definitions.h"

#include <string>
#include <boost/any.hpp>

#include <gfal_api.h>

/**
 * A deletion task, when started using a thread pool issues a delete operation
 * if retries are set another DeletionTask otherwise
 *
 * @see Gfal2Task
 */
class DeletionTask : public Gfal2Task
{

public:

    /**
     * Creates a new DeletionTask
     *
     * @param ctx : deletion task details
     */
    DeletionTask(DeletionContext const & ctx);

    /**
     * Creates a new DeletionTask from another Gfal2Task
     *
     * @param copy : a gfal2 task
     */
    DeletionTask(DeletionTask && copy) : Gfal2Task(std::move(copy)), ctx(copy.ctx) {}

    /**
     * Destructor
     */
    virtual ~DeletionTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

private:
    /// implementation of run routine for not SRM jobs
    void run_impl();

    /// deletion details
    DeletionContext const ctx;
};


#endif /* DELETIONTASK_H_ */
