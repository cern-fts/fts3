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
#ifndef QOSTRANSITIONTASK_H_
#define QOSTRANSITIONTASK_H_

#include <string>
#include <utility>
#include <boost/any.hpp>

#include "db/generic/SingleDbInstance.h"
#include "cred/DelegCred.h"

#include "Gfal2Task.h"
#include "Gfal2QoS.h"
#include "qos-daemon/context/CDMIQosTransitionContext.h"


/**
 * A QoS transition task, when started using a thread pool, issues a QoS transition operation.
 * If successful, spawns a CDMIPollTask, if not, the transition retry will be picked up by another QoSTransitionTask.
 */
class QoSTransitionTask : public Gfal2Task
{

public:

    /**
     * Creates a new QoSTransitionTask from a message_qos-transition
     *
     * @param ctx : qos-transition task details
     */
	QoSTransitionTask(const CDMIQosTransitionContext &ctx) : Gfal2Task("QOS_TRANSITION"), ctx(ctx)
    {}

    /**
     * Creates a new QoSTransitionTask from another QoSTransitionTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
	QoSTransitionTask(QoSTransitionTask && copy) : Gfal2Task(std::move(copy)), ctx(std::move(copy.ctx)) {}

    /**
     * Destructor
     */
    virtual ~QoSTransitionTask()
    {
    }

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

protected:

    /// QoS transition details
    CDMIQosTransitionContext ctx;

private:

    /// Gfal2 QoS helper
    Gfal2QoS gfal2QoS;
};


#endif // QOSTRANSITIONTASK_H_
