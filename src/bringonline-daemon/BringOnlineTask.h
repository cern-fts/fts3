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

class BringOnlineTask : public StagingTask
{

public:
	BringOnlineTask(message_bringonline ctx) : StagingTask(ctx), db(*db::DBSingleton::instance().getDBObjectInstance()) {};
	virtual ~BringOnlineTask() {}

	virtual void run(boost::any const & thread_ctx);

private:

	static time_t getPollInterval(int nPolls)
	{
	    if (nPolls > 5)
	        return 180;
	    else
	        return (2 << nPolls);
	}

	static bool checkValidProxy(const std::string& filename, std::string& message)
	{
	    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
	    return delegCredPtr->isValidProxy(filename, message);
	}

	GenericDbIfce& db;
};


#endif /* BRINGONLINETASK_H_ */
