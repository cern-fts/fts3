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

#ifndef QOS_SERVER_H_
#define QOS_SERVER_H_

#include "common/Singleton.h"
#include "common/ThreadPool.h"

#include "state/DeletionStateUpdater.h"
#include "state/StagingStateUpdater.h"
#include "state/ArchivingStateUpdater.h"
#include "task/Gfal2Task.h"
#include "task/WaitingRoom.h"

class PollTask;
class ArchivingPollTask;
class HttpPollTask;

class QoSServer: public fts3::common::Singleton<QoSServer>
{
public:
	QoSServer();
    virtual ~QoSServer();

    void start();
    void stop();
    void wait();

    DeletionStateUpdater& getDeletionStateUpdater() {
    	return deletionStateUpdater;
    }

    StagingStateUpdater& getStagingStateUpdater() {
        return stagingStateUpdater;
    }

    ArchivingStateUpdater& getArchivingStateUpdater() {
        return archivingStateUpdater;
    }

    WaitingRoom<PollTask>& getWaitingRoom() {
        return waitingRoom;
    }

    WaitingRoom<HttpPollTask>& getHttpWaitingRoom() {
        return httpWaitingRoom;
    }

    WaitingRoom<ArchivingPollTask>& getArchivingWaitingRoom() {
        return archivingWaitingRoom;
    }
private:
    std::string processName{"fts_qosdaemon"};
    boost::thread_group systemThreads;
    fts3::common::ThreadPool<Gfal2Task> threadpool;
    WaitingRoom<PollTask> waitingRoom;
    WaitingRoom<HttpPollTask> httpWaitingRoom;
    WaitingRoom<ArchivingPollTask> archivingWaitingRoom;

    DeletionStateUpdater deletionStateUpdater;
    StagingStateUpdater stagingStateUpdater;
    ArchivingStateUpdater archivingStateUpdater;
};


#endif // QOS_SERVER_H_
