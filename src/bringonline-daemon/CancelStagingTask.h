/*
 * CancelStagingTask.h
 *
 *  Created on: 2 Jul 2014
 *      Author: simonm
 */

#ifndef CANCELSTAGINGTASK_H_
#define CANCELSTAGINGTASK_H_

#include "Gfal2Task.h"

#include <boost/tuple/tuple.hpp>
#include <string>

class CancelStagingTask : public Gfal2Task
{
	// file_id, surl, token
	typedef boost::tuple<int, std::string, std::string> context_type;

public:
	CancelStagingTask(context_type const & ctx) : ctx(ctx) {}
	virtual ~CancelStagingTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

private:
    context_type ctx;

};

#endif /* CANCELSTAGINGTASK_H_ */
