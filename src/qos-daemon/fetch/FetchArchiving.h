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
#ifndef FETCHARCHIVING_H_
#define FETCHARCHIVING_H_

#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "common/ThreadPool.h"
#include "cred/DelegCred.h"

#include "../task/Gfal2Task.h"
#include "../context/ArchivingContext.h"


/**
 * Fetches the archiving transfers from DB in a separate thread
 */
class FetchArchiving
{

public:
    FetchArchiving(fts3::common::ThreadPool<Gfal2Task> & threadpool) : threadpool(threadpool) {}
    virtual ~FetchArchiving() {}

    void fetch();

    // Group by  credential ID (VO + DN) and storage endpoint
    typedef std::pair<std::string, std::string> GroupByType;

private:
    // Function that recovers from DB archiving tasks that already started
    // To be called on the server start
    void recoverStartedTasks() const;

    // Function that iterates over a set of archiving operations and groups them into batches to be scheduled
    void startArchivePollTasks(const  std::vector<ArchivingOperation>& archivingOperations) const;

    // Function to start ArchivePollTasks on the server thread pool
    void scheduleArchivePollTask(const ArchivingContext& context) const;

    fts3::common::ThreadPool<Gfal2Task> & threadpool;

};

#endif // FETCHSTAGING_H_
