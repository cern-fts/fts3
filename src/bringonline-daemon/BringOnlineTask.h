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
	BringOnlineTask(message_bringonline ctx) : StagingTask(ctx) {};
	virtual ~BringOnlineTask() {}

	virtual void run(boost::any const & thread_ctx);
};


#endif /* BRINGONLINETASK_H_ */
