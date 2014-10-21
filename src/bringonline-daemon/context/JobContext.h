/*
 * JobContext.h
 *
 *  Created on: 15 Sep 2014
 *      Author: simonm
 */

#ifndef JOBCONTEXT_H_
#define JOBCONTEXT_H_

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <map>

/**
 * Most generic Job (Deletion, Bring-online, etc.) context
 *
 * Contains: list of surls, job ID - files mapping, proxy-certificate file name
 */
class JobContext
{
public:
    /**
     * Constructor
     *
     * @param dn : user DN
     * @param vo : user VO
     * @param delegationId : delegation ID
     * @param spaceToken : space token
     */
    JobContext(std::string const & dn, std::string const & vo, std::string const & delegationId, std::string const & spaceToken = "");

    /**
     * Copy constructor
     *
     * @param jc : JobContext to be copied
     */
    JobContext(JobContext const & jc) : jobs(jc.jobs), proxy(jc.proxy), spaceToken(jc.spaceToken), urlToIDs(jc.urlToIDs) {}

    /**
     * Move constructor
     *
     * @param jc : JobContext to be moved
     */
    JobContext(JobContext && jc) :
        jobs(std::move(jc.jobs)), proxy(std::move(jc.proxy)), spaceToken(std::move(jc.spaceToken)), urlToIDs(std::move(jc.urlToIDs)) {}

    /**
     * Destructor
     */
    virtual ~JobContext() {}

    /**
     * adds another surl and file to the context
     *
     * @param surl : SURL name
     * @param jobId : job ID
     * @param fileId : file ID
     */
    void add(std::string const & surl, std::string const & jobId, int fileId);

    /**
     * Updates the state of the job (pure virtual)
     *
     * @param state : new state
     * @param reason : reason for changing the state
     * @param retry : if true the job will be retried
     */
    virtual void state_update(std::string const & state, std::string const & reason, bool retry) const = 0;

    /**
     * Checks if the proxy is valid
     *
     * @param message : the error message returned in case the proxy is not valid
     * @return : true if the proxy is valid, false otherwise
     */
    bool checkValidProxy(std::string& message) const;

    /**
     * @return : list of URLs
     */
    std::vector<char const *> getUrls() const;

    /**
     * @return : set of SURLs
     */
    std::set<std::string> getSurls() const
    {
        std::set<std::string> surls;

        std::map< std::string, std::vector<std::pair<int, std::string> > >::const_iterator it_j;
        std::vector<std::pair<int, std::string> >::const_iterator it_f;
        for (it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
            {
                for (it_f = it_j->second.begin(); it_f != it_j->second.end(); ++it_f)
                    {
                        surls.insert(it_f->second);
                    }
            }

        return surls;
    }

    void removeUrl(const std::string& url);

    /**
     * @return : proxy-certificate file name
     */
    std::string const & getProxy() const
    {
        return proxy;
    }

    /**
     * @return : space token
     */
    std::string const & getSpaceToken() const
    {
        return spaceToken;
    }

    std::string getLogMsg() const;

    /**
     * Get job and file ID for the given SURL
     */
    std::vector< std::pair<std::string, int> > getIDs(std::string const & surl) const
    {
        std::vector< std::pair<std::string, int> > ret;
        auto range = urlToIDs.equal_range(surl);
        for (auto it = range.first; it != range.second; ++it)
            {
                ret.push_back(it->second);
            }
        return ret;
    }

private:

    /**
     * Generates the proxy-certificate file name based on DN and delegation ID
     *
     * @param dn : user DN
     * @param delegationId : delegation ID
     */
    static std::string generateProxy(std::string const & dn, std::string const & delegationId);

protected:

    /// Job ID -> list of (file ID and SURL) mapping
    std::map< std::string, std::vector<std::pair<int, std::string> > > jobs;

    /// proxy-certificate file name
    std::string proxy;
    /// space token
    std::string spaceToken;
    /// URL -> (job_id, file_id)
    std::unordered_multimap< std::string, std::pair<std::string, int> > urlToIDs;
};

#endif /* JOBCONTEXT_H_ */
