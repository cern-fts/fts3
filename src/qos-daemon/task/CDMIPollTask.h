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

#include "Gfal2QoS.h"
#include "QoSTransitionTask.h"
#include "db/generic/SingleDbInstance.h"


/**
 * A poll task: checks whether a given QoS transition was successful
 *
 * If the QoS transition operation is not finished yet, spawns another CDMIPollTask.
 *
 * @see BringOnlineTask
 */
class CDMIPollTask : public QoSTransitionTask
{
public:
    /**
     *
     * @param ctx : staging context (recover from DB after crash)
     */
	CDMIPollTask(const CDMIQosTransitionContext &ctx) :
		QoSTransitionTask(ctx), nPolls(0), wait_until(0)
    {}

    /**
     * Creates a new CDMIPollTask task from a QoSTransitionTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
	CDMIPollTask(QoSTransitionTask && copy) :
		QoSTransitionTask(std::move(copy)), nPolls(0), wait_until(0)
    {}

    /**
     * Move constructor
     */
	CDMIPollTask(CDMIPollTask && copy) :
		QoSTransitionTask(std::move(copy)), nPolls(copy.nPolls), wait_until(copy.wait_until)
    {}

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
    inline bool waiting(time_t now)
    {
        return wait_until > now;
    }

private:

    /**
     * Get the interval after which next polling should be done
     *
     * @param nPolls : number of polls already done
     */
    static time_t getPollInterval(int nPolls)
    {
        return (nPolls > 9) ? 600 : (2 << nPolls);
    }

    /// number of bring online polls
    int nPolls;

    /// wait in the wait room until given time
    time_t wait_until;

    /// Gfal2 QoS helper
    Gfal2QoS gfal2QoS;
};

#endif // CDMIPOLLTASK_H_
