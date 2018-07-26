/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#ifndef CDMIPOLLTASK_H_
#define CDMIPOLLTASK_H_

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>

#include <boost/thread.hpp>

#include "db/generic/SingleDbInstance.h"

#include "QoSTransitionTask.h"


/**
 * A poll task: checks whether a given qos transition was successful
 *
 * If the qos transition operation is not finished yet spawns another CDMIPollTask.
 * If the operation fails and retries are set spawns another QoSTransitionTask.
 *
 * @see StagingTask
 * @see BringOnlineTask
 */
class CDMIPollTask : public QoSTransitionTask
{
public:
    /**
     * Creates a CDMIPollTask from StagingContext (for recovery purposes only)
     *
     * @param ctx : staging context (recover from DB after crash)
     * @param token : token that is needed for polling
     */
	CDMIPollTask(const CDMIQosTransitionContext &ctx, const std::string &token) :
		QoSTransitionTask(ctx), token(token), nPolls(0), wait_until(0)
    {
        //auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        //active_urls.insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new PollTask task from a BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
	CDMIPollTask(QoSTransitionTask && copy, const std::string &token) :
		QoSTransitionTask(std::move(copy)), token(token), nPolls(0), wait_until()
    {
    }

    /**
     * Move constructor
     */
	CDMIPollTask(CDMIPollTask && copy) :
		QoSTransitionTask(std::move(copy)), token(copy.token), nPolls(copy.nPolls), wait_until(
            copy.wait_until)
    {
    }

    /**
     * Destructor
     */
    virtual ~CDMIPollTask() {}

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(const boost::any &);

    /**
     * @return : true if the task is still waiting, false otherwise
     */
    bool waiting(time_t now)
    {
        return wait_until > now;
    }

private:

    /**
     * Gets the interval after next polling should be done
     *
     * @param nPolls : number of polls already done
     */
    static time_t getPollInterval(int nPolls)
    {
        if (nPolls > 9)
            return 600;
        else
            return (2 << nPolls);
    }

    /// the token that will be used for polling
    std::string token;

    /// number of bring online polls
    int nPolls;

    /// wait in the wait room until given time
    time_t wait_until;
};

#endif // CDMIPOLLTASK_H_
