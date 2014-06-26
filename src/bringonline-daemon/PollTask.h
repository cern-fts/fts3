/*
 * PollTask.h
 *
 *  Created on: 25 Jun 2014
 *      Author: simonm
 */

#ifndef POLLTASK_H_
#define POLLTASK_H_

#include "StagingTask.h"

#include "common/definitions.h"

#include "db/generic/SingleDbInstance.h"

class PollTask : public StagingTask
{

public:
	PollTask(message_bringonline ctx) : StagingTask(ctx) {}
	virtual ~PollTask() {}

	virtual void run(boost::any const & thread_ctx);

private:

	static time_t getPollInterval(int nPolls)
	{
	    if (nPolls > 5)
	        return 180;
	    else
	        return (2 << nPolls);
	}
};

#endif /* POLLTASK_H_ */
