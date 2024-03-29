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

#include <string>
#include <utility>

#include <boost/any.hpp>

#include <gfal_api.h>

#include "db/generic/SingleDbInstance.h"
#include "cred/DelegCred.h"

#include "Gfal2Task.h"
#include "qos-daemon/context/HttpStagingContext.h"


/**
 * A bring-online task, when started using a thread pool issues a bring online operation
 * if successful spawns a PollTask, if retries are set another BringOnlineTask otherwise
 *
 * @see StagingTask
 */
class HttpBringOnlineTask : public Gfal2Task
{

public:

    /**
     * Creates a new BringOnlineTask from a message_bringonline
     *
     * @param ctx : bring-online task details
     */
    HttpBringOnlineTask(HttpStagingContext&& copy_ctx) : Gfal2Task("HTTP_STAGING"), ctx(std::move(copy_ctx))
    {
        // set up the space token
        setSpaceToken(ctx.getSpaceToken());
        // set the proxy certificate
        setProxy(ctx);
        // add urls to active
        auto surls = ctx.getSurls();
        boost::unique_lock<boost::shared_mutex> lock(mx);
        active_urls.insert(surls.begin(), surls.end());
    }

    /// Copy constructor (deleted)
    HttpBringOnlineTask(const HttpBringOnlineTask& copy) = delete;

    /**
     * Creates a new BringOnlineTask from another BringOnlineTask
     *
     * @param copy : a staging task (stills the gfal2 context of this object)
     */
    HttpBringOnlineTask(HttpBringOnlineTask && copy) : Gfal2Task(std::move(copy)), ctx(std::move(copy.ctx)) {}

    /**
     * Destructor
     */
    virtual ~HttpBringOnlineTask()
    {
        if (gfal2_ctx)
            cancel(ctx.getSurls());
    }

    /**
     * The routine is executed by the thread pool
     */
    virtual void run(boost::any const &);

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

protected:

    /// staging details
    HttpStagingContext ctx;
    /// prevents concurrent access to active_tokens
    static boost::shared_mutex mx;
    /// set of tokens (and respective URLs) for ongoing bring-online jobs
    static std::set<std::pair<std::string, std::string>> active_urls;
};
