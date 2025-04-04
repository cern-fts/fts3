/*
 * Copyright (c) CERN 2013-2019
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
#ifndef ARCHIVINGPOLLTASK_H_
#define ARCHIVINGPOLLTASK_H_

#include <algorithm>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>

#include <boost/thread.hpp>

#include "db/generic/SingleDbInstance.h"
#include "Gfal2Task.h"
#include "../context/ArchivingContext.h"


/**
 * An archiving poll task: checks whether a list of URLs have been archived
 *
 * If the archive operation is not finished yet spawns another ArchivingPollTask.
 * If the operation fails and retries are set spawns another ArchivingPollTask.
 *
 * @see PollTask
 */
class ArchivingPollTask : public Gfal2Task
{
public:
    /**
     * Creates a ArchivingPollTask from ArchiveContext (for recovery purposes only)
     *
     * @param ctx : staging context (recover from DB after crash)
     */
	ArchivingPollTask(const ArchivingContext &ctx) : Gfal2Task("ARCHIVING"), ctx(ctx), nPolls(0), wait_until(0)
    {
        // set the proxy certificate
        setProxy(ctx);
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
    }

    /**
     * Creates a new ArchivingPollTask task from a ArchivingTask
     *
     * @param copy : a archive task (stills the gfal2 context of this object)
     */
    ArchivingPollTask(ArchivingPollTask &&copy) : Gfal2Task(std::move(copy)), ctx(std::move(copy.ctx)),
        nPolls(copy.nPolls), wait_until(copy.wait_until)
    {
    }

    /**
     * Destructor
     */
    virtual ~ArchivingPollTask() {
         if (gfal2_ctx)
            cancel(ctx.getSurls());
    }

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

    static void cancel(const std::set<std::pair<std::string, std::string> > &urls)
        {
            if (urls.empty()) return;
            boost::unique_lock<boost::shared_mutex> lock(mx);
            auto begin = active_urls.lower_bound(*urls.begin());
            auto end   = active_urls.upper_bound(*urls.rbegin());
            for (auto it = begin; it != end;) {
                if (urls.count(*it)) {
                    active_urls.erase(it++);
                }
                else {
                    ++it;
                }
            }
        }

private:
    /// check each transfer for timeout
    void handle_timeouts();

    /// aborts the operation for the given URLs
    void abort(std::set<std::string> const & urls, bool report = true);

    /**
     * Gets the interval after next polling should be done
     *
     * @param nPolls : number of polls already done
     * @param archivePollInterval : Configurable interval for tape archive polling
     */
    static time_t getPollInterval(int nPolls, int archivePollInterval)
    {
        if (nPolls > 1)
            return archivePollInterval;  // Subsequent polls follow the configured interval
        else
            return 300;     // First retry after 5 minutes
    }

    /// archiving details
    ArchivingContext ctx;

    /// number of archive task polls
    int nPolls;

    /// wait in the wait room until given time
    time_t wait_until;

    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    
    /// set of jobid (and respective URLs) for ongoing archiving 
    static std::set<std::pair<std::string, std::string>> active_urls;
};

#endif // ARCHIVINGPOLLTASK_H_
