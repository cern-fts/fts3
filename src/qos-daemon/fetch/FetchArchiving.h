/*
 * Copyright (c) CERN 2013-2012
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

#include <map>
#include <string>
#include <vector>

#include "common/ThreadPool.h"
#include "qos-daemon/task/Gfal2Task.h"
#include "qos-daemon/context/ArchivingContext.h"


/**
 * Fetches the archiving transfers from DB in a separate thread
 */
class FetchArchiving
{

public:
    FetchArchiving(fts3::common::ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}

    virtual ~FetchArchiving() = default;

    void fetch();

    /// Group by credential ID (VO + DN) and storage endpoint
    typedef std::pair<std::string, std::string> GroupByType;

private:
    /**
     * Function that recovers from DB archiving tasks that already started.
     * To be called on the server start
     */
    void recoverStartedTasks() const;

    /**
     * Function that iterates over a set of archiving operations and groups them into batches to be scheduled
     *
     * @param archivingOperations - vector for archiving operations retrieved from the database
     * @param recovery - signal if the scheduled task is part of start-up recovery or not (default false)
     */
    void startArchivePollTasks(const std::vector<ArchivingOperation>& archivingOperations,
                               bool recovery = false) const;

    /**
     * Function to start ArchivePollTasks on the server thread pool
     *
     * @param context - the archiving context object
     * @param recovery - signal if the scheduled task is part of start-up recovery or not
     */
    void scheduleArchivePollTask(ArchivingContext& context, bool recovery) const;

    fts3::common::ThreadPool<Gfal2Task> & threadpool;
};
