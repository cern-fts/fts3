/*
 * BringOnlineTask.h
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#ifndef BRINGONLINETASK_H_
#define BRINGONLINETASK_H_


#include "StagingTask.h"

#include "common/definitions.h"

#include "db/generic/SingleDbInstance.h"

#include "cred/DelegCred.h"

#include <string>

#include <boost/scoped_ptr.hpp>
#include <boost/any.hpp>

#include <gfal_api.h>

/**
 * A bring-online task, when started using a thread pool issues a bring online operation
 * if successful spawns a PollTask, if retries are set another BringOnlineTask otherwise
 *
 * @see StagingTask
 */
class BringOnlineTask : public StagingTask
{

public:

    /**
     * Creates a new BringOnlineTask from a message_bringonline
     *
     * @param ctx : bring-online task details
     */
    BringOnlineTask(context_type const & ctx, std::string const & proxy);

    /**
     * Creates a new BringOnlineTask from another StagingTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    BringOnlineTask(StagingTask & copy) : StagingTask(copy) {}

    /**
     * Destructor
     */
    virtual ~BringOnlineTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

    /**
     * Checks if a proxy is valid
     *
     * @param filename : file name of the proxy
     * @param message : potential error message
     */
    static bool checkValidProxy(std::string const & filename, std::string& message)
    {
        boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
        return delegCredPtr->isValidProxy(filename, message);
    }

private:

    /**
     * sets the proxy
     */
    void setProxy();

    /// path to the proxy certificate
    std::string proxy;
};


#endif /* BRINGONLINETASK_H_ */
