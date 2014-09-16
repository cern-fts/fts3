/*
 * StagingContext.h
 *
 *  Created on: 10 Jul 2014
 *      Author: simonm
 */

#ifndef STAGINGCONTEXT_H_
#define STAGINGCONTEXT_H_

#include "JobContext.h"
#include "StagingStateUpdater.h"

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
        src_space_token
    };

    StagingContext(context_type const & ctx) :
        JobContext(boost::get<dn>(ctx), boost::get<vo>(ctx), boost::get<dlg_id>(ctx), boost::get<src_space_token>(ctx)),
        pinlifetime(), bringonlineTimeout()
    {
        add(ctx);
    }

    StagingContext(StagingContext const & copy) :
        JobContext(copy),
        pinlifetime(copy.pinlifetime), bringonlineTimeout(copy.bringonlineTimeout) {}

    virtual ~StagingContext() {}

    void add(context_type const & ctx);

    std::map< std::string, std::vector<int> > const & getJobs() const
    {
        return jobs;
    }

    void state_update(std::string const & state, std::string const & reason, bool retry) const
    {
        static StagingStateUpdater & state_update = StagingStateUpdater::instance();
        state_update(jobs, state, reason, retry);
    }

    void state_update(std::string const & token)
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

private:

    int pinlifetime;
    int bringonlineTimeout;
};

#endif /* STAGINGCONTEXT_H_ */
