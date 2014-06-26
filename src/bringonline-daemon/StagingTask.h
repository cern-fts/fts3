/*
 * StagingTask.h
 *
 *  Created on: 24 Jun 2014
 *      Author: simonm
 */

#ifndef StagingTask_H_
#define StagingTask_H_

#include "common/definitions.h"
#include "common/logger.h"

#include "db/generic/SingleDbInstance.h"

#include "cred/DelegCred.h"

#include <boost/any.hpp>

#include <gfal_api.h>

using namespace FTS3_COMMON_NAMESPACE;

class StagingTask
{

public:
    StagingTask(message_bringonline ctx) : ctx(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()) {}
    virtual ~StagingTask() {}
    virtual void run(boost::any const &) = 0;

    bool retryTransfer(int errorNo, std::string const & category, std::string const & message);

    void setProxy(gfal2_context_t handle);

    message_bringonline const & get() const
    {
        return ctx;
    }

    static bool checkValidProxy(const std::string& filename, std::string& message)
    {
        boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
        return delegCredPtr->isValidProxy(filename, message);
    }

protected:
    message_bringonline ctx;
    GenericDbIfce& db;
};

#endif /* StagingTask_H_ */
