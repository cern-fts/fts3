/*
 * DeletionContext.h
 *
 *  Created on: 17 Jul 2014
 *      Author: roiser
 */

#ifndef DELETIONCONTEXT_H_
#define DELETIONCONTEXT_H_

#include "cred/DelegCred.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <set>

#include <boost/tuple/tuple.hpp>

class DeletionContext
{
    friend class DeletionTask;
    friend class PollTask;

public:

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

    DeletionContext()  {}

    virtual ~DeletionContext() {}

    void add(context_type const & ctx);

    std::map< std::string, std::vector<int> > const & getJobs() const
    {
        return jobs;
    }

    std::vector<char const *> getUrls() const;

    std::string const & getProxy() const
    {
        return proxy;
    }

    int getBringonlineTimeout() const
    {
        return bringonlineTimeout;
    }

    std::string getLogMsg() const;

    /**
     * Checks if a proxy is valid
     *
     * @param filename : file name of the proxy
     * @param message : potential error message
     */
    static bool checkValidProxy(std::string const & filename, std::string& message)
    {
        std::unique_ptr<DelegCred> delegCredPtr(new DelegCred);
        return delegCredPtr->isValidProxy(filename, message);
    }

private:

    static std::string generateProxy(std::string const & dn, std::string const & dlg_id)
    {
        std::unique_ptr<DelegCred> delegCredPtr(new DelegCred);
        return delegCredPtr->getFileName(dn, dlg_id);
    }

    std::set<std::string> mutable surls;
    int bringonlineTimeout;
    std::string delegationId;
    std::map< std::string, std::vector<int> > jobs;
    std::string proxy;
};

#endif /* DELETIONCONTEXT_H_ */
