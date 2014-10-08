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

    enum {vo_name, source_url, job_id, file_id, user_dn, cred_id};

    /**
     * Constructor (adds the first for deletion)
     *
     * @param ctx : the tuple with data necessary to initialise an instance
     */
    DeletionContext(context_type const & ctx) :
        JobContext(boost::get<user_dn>(ctx), boost::get<vo_name>(ctx), boost::get<cred_id>(ctx))
    {
        add(ctx);
    }

    /**
     * Copy constructor
     *
     * @param copy : other DeletionContext instance
     */
    DeletionContext(DeletionContext const & copy) : JobContext(copy), srm_jobs(copy.srm_jobs), urlToIDs(copy.urlToIDs) {}
   

    /**
     * Move constructor
     *
     * @param copy : the context to be moved
     */
    DeletionContext(DeletionContext && copy) :
        JobContext(std::move(copy)), srm_jobs(std::move(copy.srm_jobs)), urlToIDs(std::move(copy.urlToIDs)) {}

    /**
     * Destructor
     */
    virtual ~DeletionContext() {}

    /**
     * Add another file for deletion
     *
     * @param ctx : file for deletion
     */
    void add(context_type const & ctx);

    /**
     * Asynchronous update of a single transfer-file within a job
     */
    void state_update(std::string const & job_id, int file_id, std::string const & state, std::string const & reason, bool retry) const
    {
        static DeletionStateUpdater & state_update = DeletionStateUpdater::instance();
        state_update(job_id, file_id, state, reason, retry);
    }

    /**
     * Bulk state update implementation for srm endpoints
     */
    void state_update(std::string const & state, std::string const & reason, bool retry) const
    {
        static DeletionStateUpdater & state_update = DeletionStateUpdater::instance();
        state_update(srm_jobs, state, reason, retry);
    }

    /**
     * @return : list of SRM URLs
     */
    std::vector<char const *> getSrmUrls() const;

    /**
     * @return : map with gsiftp jobs
     */
    std::map< std::string, std::vector<std::pair<int, std::string> > > const & getGsiftpJobs() const
    {
        return jobs;
    }

    std::pair<std::string, int> getIDs(std::string const & surl) const
    {
        std::pair<std::string, int> ret;
        std::map< std::string, std::pair<std::string, int> >::const_iterator it = urlToIDs.find(surl);
        if (it != urlToIDs.end()) ret = it->second;
        return ret;
    }

private:
    /// Job ID -> list of (file ID and SURL) mapping
    std::map< std::string, std::vector<std::pair<int, std::string> > > srm_jobs;
    /// URL -> (job_id, file_id)
    std::map< std::string, std::pair<std::string, int> > urlToIDs;
};

#endif /* DELETIONCONTEXT_H_ */
