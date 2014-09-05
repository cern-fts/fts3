/*
 * DeletionTask.h
 *
 *  Created on: 17 July 2014
 *      Author: roiser
 */

#ifndef DELETIONTASK_H_
#define DELETIONTASK_H_


#include "Gfal2Task.h"

#include "DeletionContext.h"

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

    // typedefs for convenience
    // vo, dn ,se, source_space_token
    typedef std::tuple<std::string, std::string, std::string> key_type;

    enum
    {
        vo,
        dn,
        se
    };

public:

    /**
     * Creates a new DeletionTask
     *
     * @param ctx : deletion task details
     */
    DeletionTask(std::pair<key_type, DeletionContext> const & ctx);

    /**
     * Creates a new DeletionTask from another Gfal2Task
     *
     * @param copy : a gfal2 task
     */
    DeletionTask(Gfal2Task & copy) : Gfal2Task(copy) {}

    /**
     * Destructor
     */
    virtual ~DeletionTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

private:

    /**
     * sets the proxy
     */
    void setProxy();
    /// staging details
    DeletionContext const ctx;

};


#endif /* DELETIONTASK_H_ */
