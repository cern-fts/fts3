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
#include <utility>

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

    // typedefs for convenience
    // vo, dn ,se, source_space_token
    typedef std::tuple<std::string, std::string, std::string, std::string> key_type;

    enum
    {
        vo,
        dn,
        se,
        space_token
    };

public:

    /**
     * Creates a new BringOnlineTask from a message_bringonline
     *
     * @param ctx : bring-online task details
     */
    BringOnlineTask(std::pair<key_type, StagingContext> const & ctx);

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

private:

    /**
     * sets the proxy
     */
    void setProxy();
};


#endif /* BRINGONLINETASK_H_ */
