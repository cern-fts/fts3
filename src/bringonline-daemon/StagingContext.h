/*
 * StagingContext.h
 *
 *  Created on: 10 Jul 2014
 *      Author: simonm
 */

#ifndef STAGINGCONTEXT_H_
#define STAGINGCONTEXT_H_

#include "cred/DelegCred.h"

#include <vector>
#include <string>
#include <map>
#include <memory>

#include <boost/tuple/tuple.hpp>

class StagingContext
{

public:

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

    StagingContext() : pinlifetime(28800), bringonlineTimeout(28800) {}

    StagingContext(StagingContext const & copy) :
        surls(copy.surls),
        pinlifetime(copy.pinlifetime),
        bringonlineTimeout(copy.bringonlineTimeout),
        delegationId(copy.delegationId),
        jobs(copy.jobs),
        proxy(copy.proxy)
    {

    }

    virtual ~StagingContext() {}

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

    int getPinlifetime() const
    {
        return pinlifetime;
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

    std::vector<std::string> surls;
    int pinlifetime;
    int bringonlineTimeout;
    std::string delegationId;
    std::map< std::string, std::vector<int> > jobs;
    std::string proxy;
};

#endif /* STAGINGCONTEXT_H_ */
