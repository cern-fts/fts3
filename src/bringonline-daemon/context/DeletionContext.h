/*
 * DeletionContext.h
 *
 *  Created on: 17 Jul 2014
 *      Author: roiser
 */

#ifndef DELETIONCONTEXT_H_
#define DELETIONCONTEXT_H_

#include "JobContext.h"
#include "state/DeletionStateUpdater.h"

#include "cred/DelegCred.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

#include <boost/tuple/tuple.hpp>

class DeletionContext : public JobContext
{

public:

    using JobContext::add;

    // typedef for convenience
    // vo_name, source_url, job_id, file_id, user_dn, cred_id
    typedef boost::tuple<std::string, std::string, std::string, int, std::string, std::string> context_type;

    enum
    {
        vo_name,
        source_url,
        job_id,
        file_id,
        user_dn,
        cred_id
    };

    DeletionContext(context_type const & ctx) :
        JobContext(boost::get<user_dn>(ctx), boost::get<vo_name>(ctx), boost::get<cred_id>(ctx))
    {
        add(ctx);
    }

    DeletionContext(DeletionContext const & copy) : JobContext(copy) {}

    virtual ~DeletionContext() {}

    void add(context_type const & ctx);

    void state_update(std::string const & state, std::string const & reason, bool retry) const
    {
        static DeletionStateUpdater & state_update = DeletionStateUpdater::instance();
        state_update(jobs, state, reason, retry);
    }
};

#endif /* DELETIONCONTEXT_H_ */
