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

/**
 * A poll task: checks whether a given bring-online operation was successful
 *
 * If the bring-online operation is not finished yet spawns another PollTask.
 * If the operation fails and retries are set spawns another BringOnlineTask.
 *
 * @see StagingTask
 * @see BringOnlineTask
 */
class PollTask : public StagingTask
{

public:

	/**
	 * Creates a new PollTask task from another StagingTask
	 *
	 * @param copy : a staging task (stills the gfal2 context of this object)
	 */
	PollTask(StagingTask & copy) : StagingTask(copy) {}

	/**
	 * Destructor
	 */
	virtual ~PollTask() {}

	/**
	 * The routine is executed by the thread pool
	 */
	virtual void run(boost::any const &);

private:

	/**
	 * Gets the interval after next polling should be done
	 *
	 * @param nPolls : number of polls already done
	 */
	static time_t getPollInterval(int nPolls)
	{
	    if (nPolls > 5)
	        return 180;
	    else
	        return (2 << nPolls);
	}
};

#endif /* POLLTASK_H_ */
