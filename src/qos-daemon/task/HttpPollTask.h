/*
 * Copyright (c) CERN 2022
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

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>

#include <boost/thread.hpp>

#include "db/generic/SingleDbInstance.h"

#include "HttpBringOnlineTask.h"

/**
 * A poll task: checks whether a given bring-online operation was successful
 *
 * If the bring-online operation is not finished yet spawns another PollTask.
 * If the operation fails and retries are set spawns another BringOnlineTask.
 *
 * @see StagingTask
 * @see BringOnlineTask
 */
class HttpPollTask : public HttpBringOnlineTask
{
public:
    /**
     * Creates a PollTask from StagingContext (for recovery purposes only)
     *
     * @param ctx : staging context (recover from DB after crash)
     * @param token : token that is needed for polling
     */
    HttpPollTask(HttpStagingContext&& copy_ctx, const std::string& token) :
            HttpBringOnlineTask(std::move(copy_ctx)), token(token), nPolls(0), wait_until(0)
    {
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new PollTask task from a BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    HttpPollTask(HttpBringOnlineTask && copy, const std::string &token) :
            HttpBringOnlineTask(std::move(copy)), token(token), nPolls(0), wait_until()
    {
    }

    /**
     * Move constructor
     */
    HttpPollTask(HttpPollTask && copy) :
            HttpBringOnlineTask(std::move(copy)), token(copy.token), nPolls(copy.nPolls), wait_until(
            copy.wait_until)
    {
    }

    /**
     * Destructor
     */
    virtual ~HttpPollTask() {}

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
    /// checks if the bring online task was cancelled and removes those URLs that were from the context
    void handle_canceled();

    /// checks if the bring online task timed-out and removes respective URLs from the context
    bool timeout_occurred();

    /// aborts the bring online operation for the given URLs
    void abort(std::set<std::string> const & urls, bool report = true);

    /**
     * Gets the interval after next polling should be done
     *
     * @param nPolls : number of polls already done
     * @param stagingPollInterval : Configurable interval for tape stage polling
     */
    static time_t getPollInterval(int nPolls, int stagingPollInterval)
    {
        if (nPolls > 1)
            return stagingPollInterval;  // Subsequent polls follow the configured interval
        else
            return 300;     // First retry after 5 minutes
    }

    /// the token that will be used for polling
    std::string token;

    /// number of bring online polls
    int nPolls;

    /// wait in the wait room until given time
    time_t wait_until;
};
