/*
 * RestDelegator.h
 *
 *  Created on: 26 Aug 2014
 *      Author: simonm
 */

#ifndef RESTDELEGATOR_H_
#define RESTDELEGATOR_H_

#include "ProxyCertificateDelegator.h"

namespace fts3
{
namespace cli
{

class RestDelegator : public ProxyCertificateDelegator
{

public:
    RestDelegator(std::string const & endpoint, std::string const & delegationId, long userRequestedDelegationExpTime, std::string const & capath, std::string const & proxy) :
        ProxyCertificateDelegator(endpoint, delegationId, userRequestedDelegationExpTime, proxy), capath(capath)
    {

    }

    virtual ~RestDelegator() {}

private:

    /// @return : expiration time of the credential on server, or uninitialised optional if credential does not exist
    boost::optional<time_t> getExpirationTime();

    /**
     * Does the delegation (either using soap or rest, implementation specific)
     *
     * @param requestProxyDelegationTime : expiration time requested for the proxy
     * @param renew : renew flag
     */
    void doDelegation(time_t requestProxyDelegationTime, bool renew) const;

    std::string const capath;
};

} /* namespace cli */
} /* namespace fts3 */

#endif /* RESTDELEGATOR_H_ */
