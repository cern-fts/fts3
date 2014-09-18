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
    JobContext(JobContext const & jc) : jobs(jc.jobs), proxy(jc.proxy), spaceToken(jc.spaceToken) {}

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
};

#endif /* JOBCONTEXT_H_ */
