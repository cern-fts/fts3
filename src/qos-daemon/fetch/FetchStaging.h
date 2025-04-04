/*
 * Copyright (c) CERN 2013-2025
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

#include <string>
#include <tuple>
#include <vector>

#include "common/ThreadPool.h"
#include "qos-daemon/task/Gfal2Task.h"
#include "qos-daemon/context/StagingContext.h"


/**
 * Fetches the staging jobs from DB in a separate thread
 */
class FetchStaging
{

public:
    FetchStaging(fts3::common::ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}
    virtual ~FetchStaging() {}

    void fetch();

    // Group by credential ID (VO + DN), storage endpoint and space token
    typedef std::tuple<std::string, std::string, std::string> GroupByType;

private:
    // Function that recovers from DB staging tasks that already started
    // To be called on the daemon start
    void recoverStartedTasks() const;

    // Function that iterates over a set of staging operations and groups them into batches to be scheduled
    void startBringonlineTasks(const std::vector<StagingOperation>& stagingOperations) const;

    // Function to start HTTPBringOnlineTask & BringOnlineTask on the QoS thread pool
    void scheduleBringonlineTask(StagingContext& context) const;

    // Function to start HTTPPollTask & PollTask on the QoS thread pool
    void schedulePollTask(StagingContext& context, const std::string& token) const;

    fts3::common::ThreadPool<Gfal2Task> & threadpool;
    boost::posix_time::time_duration stagingSchedulingInterval;
};
