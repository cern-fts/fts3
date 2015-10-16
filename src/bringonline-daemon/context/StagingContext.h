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

#ifndef STAGINGCONTEXT_H_
#define STAGINGCONTEXT_H_

#include "JobContext.h"
#include "../state/StagingStateUpdater.h"

#include "cred/DelegCred.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

#include <boost/tuple/tuple.hpp>

class StagingContext : public JobContext
{

public:

    using JobContext::add;

    // typedef for convenience
    typedef boost::tuple<std::string, std::string, std::string, int, int, int, std::string, std::string, std::string> context_type;

    enum
    {
        vo,
        surl,
        job_id,
        file_id,
        copy_pin_lifetime,
        bring_online_timeout,
        dn,
        dlg_id,
        src_space_token,
        timestamp
    };

    StagingContext(context_type const & ctx) :
        JobContext(boost::get<dn>(ctx), boost::get<vo>(ctx), boost::get<dlg_id>(ctx), boost::get<src_space_token>(ctx)),
        pinlifetime(boost::get<copy_pin_lifetime>(ctx)), bringonlineTimeout(boost::get<bring_online_timeout>(ctx))
    {
        add(ctx);
        start_time = time(0);
    }

    StagingContext(StagingContext const & copy) :
        JobContext(copy),
        pinlifetime(copy.pinlifetime), bringonlineTimeout(copy.bringonlineTimeout), start_time(copy.start_time) {}

    StagingContext(StagingContext && copy) :
        JobContext(std::move(copy)),
        pinlifetime(copy.pinlifetime), bringonlineTimeout(copy.bringonlineTimeout), start_time(copy.start_time) {}

    virtual ~StagingContext() {}

    void add(context_type const & ctx);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void updateState(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry) const
    {
        static StagingStateUpdater & state_update = StagingStateUpdater::instance();
        state_update(job_id, file_id, state, reason, retry);
    }

    void updateState(std::string const & state, std::string const & reason, bool retry) const
    {
        static StagingStateUpdater & state_update = StagingStateUpdater::instance();
        state_update(jobs, state, reason, retry);
    }

    void updateState(std::string const & token)
    {
        static StagingStateUpdater & state_update = StagingStateUpdater::instance();
        state_update(jobs, token);
    }

    int getBringonlineTimeout() const
    {
        return bringonlineTimeout;
    }

    int getPinlifetime() const
    {
        return pinlifetime;
    }

    bool is_timeouted();

    std::set<std::string> for_abortion(std::set<std::pair<std::string, std::string>> const &);

private:

    int pinlifetime;
    int bringonlineTimeout;
    /// (jobID, fileID) -> submission time
    time_t start_time;
};

#endif /* STAGINGCONTEXT_H_ */
