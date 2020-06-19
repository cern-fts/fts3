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
#ifndef CDMIQOSTRANSITIONCONTEXT_H_
#define CDMIQOSTRANSITIONCONTEXT_H_


#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "JobContext.h"
#include "qos-daemon/QoSServer.h"
#include "qos-daemon/task/WaitingRoom.h"
#include "qos-daemon/state/StagingStateUpdater.h"
#include "db/generic/QosTransitionOperation.h"

class CDMIQosTransitionContext
{

public:

    CDMIQosTransitionContext(QoSServer &qosServer):
        stateUpdater(qosServer.getStagingStateUpdater()), waitingRoom(qosServer.getCDMIWaitingRoom())
    {
        startTime = time(0);
    }

    CDMIQosTransitionContext(const CDMIQosTransitionContext &copy) :
        stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom),
        filesToTransition(copy.filesToTransition), errorCount(copy.errorCount), startTime(copy.startTime)
    {}

    CDMIQosTransitionContext(CDMIQosTransitionContext && copy) :
        stateUpdater(copy.stateUpdater), waitingRoom(copy.waitingRoom),
        filesToTransition(std::move(copy.filesToTransition)), errorCount(std::move(copy.errorCount)),
        startTime(copy.startTime)
    {}

    virtual ~CDMIQosTransitionContext() {}

    void add(const QosTransitionOperation &qosTransitionOp) {
        if (qosTransitionOp.valid()) {
            filesToTransition[qosTransitionOp.jobId].push_back(qosTransitionOp);
        }
    }

    WaitingRoom<CDMIPollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    int incrementErrorCountForSurl(const std::string &surl) {
        return (errorCount[surl] += 1);
    }

    /**
     * Synchronous update of the state of a QoS transition operation
     */
    void cdmiUpdateFileStateToFinished(const std::string &jobId, uint64_t fileId) const
    {
        stateUpdater.cdmiUpdateFileStateToFinished(jobId, fileId);
    }

    void cdmiUpdateFileStateToFailed(const std::string &jobId, uint64_t fileId, const std::string &reason = "") const
    {
        stateUpdater.cdmiUpdateFileStateToFailed(jobId, fileId, reason);
    }

    void cdmiGetFilesForQosRequestSubmitted(std::vector<QosTransitionOperation> &qosTranstionOps, const std::string& qosOp) const
    {
        stateUpdater.cdmiGetFilesForQosRequestSubmitted(qosTranstionOps, qosOp);
    }

    bool cdmiUpdateFileStateToQosRequestSubmitted(const std::string &jobId, uint64_t fileId) const
    {
        return stateUpdater.cdmiUpdateFileStateToQosRequestSubmitted(jobId, fileId);
    }

    /**
     * @return  Set of QoSTransitionOperations
     */
    std::set<QosTransitionOperation> getSurls() const
    {
    	std::set<QosTransitionOperation> surls;

    	for (auto it_t = filesToTransition.begin(); it_t != filesToTransition.end(); ++it_t) {
        	for (auto it_f = it_t->second.begin(); it_f != it_t->second.end(); ++it_f) {
        		surls.insert(*it_f);
        	}
        }

        return surls;
    }

private:
    StagingStateUpdater &stateUpdater;
    WaitingRoom<CDMIPollTask> &waitingRoom;
    /// Job ID -> QoSTransition details
    std::map<std::string, std::vector<QosTransitionOperation>> filesToTransition;
    std::map<std::string, int> errorCount;
    time_t startTime;
};

#endif // CDMIQOSTRANSITIONCONTEXT_H_
