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

#include <string>

#include <boost/any.hpp>

#include <gfal_api.h>

using namespace FTS3_COMMON_NAMESPACE;

/**
 * An abstract base class for staging tasks: bringing-online and polling
 */
class StagingTask
{

public:
	/**
	 * Creates a new StagingTask from a message_bringonline
	 *
	 * @param ctx : staging task details
	 */
	StagingTask(message_bringonline ctx) : ctx(ctx), gfal2_ctx(0), db(*db::DBSingleton::instance().getDBObjectInstance()) {}

	/**
	 * Creates a new StagingTask from another StagingTask
	 *
	 * @param copy : a staging task (stills the gfal2 context of this object!)
	 */
	StagingTask(StagingTask & copy) : ctx(copy.ctx), gfal2_ctx(copy.gfal2_ctx), db(*db::DBSingleton::instance().getDBObjectInstance())
	{
		copy.gfal2_ctx = 0;
	}

	/**
	 * Destructor
	 *
	 * (RAII style handling of gfal2 context)
	 */
	virtual ~StagingTask()
	{
		if(gfal2_ctx) gfal2_context_free(gfal2_ctx);
	}

	/**
	 * The routine is executed by the thread pool
	 */
	virtual void run(boost::any const &) = 0;

	/**
	 * Check if the task should be retried
	 *
	 * @param errorNo : error code
	 * @param category : error category
	 * @param message : error message
	 */
	bool retryTransfer(int errorNo, std::string const & category, std::string const & message);

	/**
	 * Creates a prototype for all gfal2 contexts that will be created
	 *
	 * @infosys : name of the information system
	 */
	static void createPrototype(std::string const & infosys)
	{
		StagingTask::infosys = infosys;
	}

	/**
	 * @return : task details
	 */
	message_bringonline const & get() const
	{
		return ctx;
	}

	/**
	 * Checks if a proxy is valid
	 *
	 * @param filename : file name of the proxy
	 * @param message : potential error message
	 */
	static bool checkValidProxy(const std::string& filename, std::string& message)
	{
	    boost::scoped_ptr<DelegCred> delegCredPtr(new DelegCred);
	    return delegCredPtr->isValidProxy(filename, message);
	}

protected:

	/**
	 * sets the proxy
	 */
	void setProxy();

	/// the infosys used to create all gfal2 contexts
	static std::string infosys;

	/// staging details
	message_bringonline ctx;
	/// gfal2 context
	gfal2_context_t gfal2_ctx;
	/// DB interface
	GenericDbIfce& db;
};

#endif /* StagingTask_H_ */
