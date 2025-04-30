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
#ifndef JOBCONTEXT_H_
#define JOBCONTEXT_H_

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <glib.h>

class JobError {
public:
    JobError();
    JobError(const std::string &operation, const GError *error);
    JobError(const std::string &operation, int code, const std::string &error);
    JobError(const JobError&);

    ~JobError();

    std::string String() const;
    virtual bool IsRecoverable() const;

private:
    std::string operation;
    GError *error;
};

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
    JobContext(const std::string &dn, const std::string &vo, const std::string &delegationId,
        const std::string &spaceToken = "");

    /**
     * Copy constructor
     *
     * @param jc : JobContext to be copied
     */
    JobContext(const JobContext &jc) :
        jobs(jc.jobs), proxy(jc.proxy), userDn(jc.userDn), delegationId(jc.delegationId),
        spaceToken(jc.spaceToken), urlToIDs(jc.urlToIDs)
    {}

    /**
     * Move constructor
     *
     * @param jc : JobContext to be moved
     */
    JobContext(JobContext && jc) :
        jobs(std::move(jc.jobs)), proxy(std::move(jc.proxy)),
        userDn(std::move(jc.userDn)), delegationId(std::move(jc.delegationId)),
        spaceToken(std::move(jc.spaceToken)), urlToIDs(std::move(jc.urlToIDs))
    {}

    /**
     * Destructor
     */
    virtual ~JobContext() {}

    /**
     * Adds another surl and file to the context
     *
     * @param surl : SURL name
     * @param jobId : job ID
     * @param fileId : file ID
     */
    void add(const std::string& surl, const std::string& jobId, uint64_t fileId);

    /**
     * Updates the state of the job (pure virtual)
     *
     * @param state : new state
     * @param error : cause of failure, if any
     */
    virtual void updateState(const std::string &state, const JobError &error) const = 0;

    /**
     * Checks if the proxy is valid
     *
     * @param message : the error message returned in case the proxy is not valid
     * @return : true if the proxy is valid, false otherwise
     */
    bool checkValidProxy(std::string& message) const;

    /**
     * @return : set of URLs
     * Using std::set we make sure we do not get duplicates
     */
    std::set<std::string> getUrls() const;

    /**
     * @return : number of URLs
     */
    size_t getNbUrls() const
    {
        return urlToIDs.size();
    }

    /**
     * @return  Set of pairs <jobId, surl>
     */
    std::set<std::pair<std::string, std::string>> getSurls() const
    {
        std::set<std::pair<std::string, std::string>> surls;
        for (auto it_j = jobs.begin(); it_j != jobs.end(); ++it_j)
            for (auto it_u = it_j->second.begin(); it_u != it_j->second.end(); ++it_u)
                surls.insert({it_j->first, it_u->first});
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
     * Re-download the proxy certificate associated with this Job Context from the database
     */
    void refreshProxy() const;

    /**
     * Get job and file ID for the given SURL
     */
    std::vector< std::pair<std::string, uint64_t> > getIDs(const std::string &surl) const
    {
        std::vector< std::pair<std::string, uint64_t> > ret;
        auto range = urlToIDs.equal_range(surl);
        for (auto it = range.first; it != range.second; ++it) {
            ret.push_back(it->second);
        }
        return ret;
    }

protected:
    /**
     * Checks if the SURL, jobId and fileId are valid
     */
    bool isValidOp(const std::string& url, const std::string& jobId, uint64_t fileId);

    /// Job ID -> URL -> list of file IDs
    std::map< std::string, std::map<std::string, std::vector<uint64_t> > > jobs;
    /// proxy-certificate file name
    std::string proxy;
    /// user DN
    std::string userDn;
    /// delegation ID
    std::string delegationId;
    /// space token
    std::string spaceToken;
    /// URL -> (job_id, file_id)
    std::unordered_multimap< std::string, std::pair<std::string, uint64_t> > urlToIDs;
};

#endif // JOBCONTEXT_H_
