/*
 * DeletionTask.h
 *
 *  Created on: 17 July 2014
 *      Author: roiser
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

    /// implementation of run routine for SRM jobs
    void run_srm_impl();

    /// implementation of run routine for not SRM jobs
    void run_impl();

    /// deletion details
    DeletionContext const ctx;
};


#endif /* DELETIONTASK_H_ */
