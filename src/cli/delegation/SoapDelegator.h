/*
 * SoapDelegator.h
 *
 *  Created on: 26 Aug 2014
 *      Author: simonm
 */

#ifndef SOAPDELEGATOR_H_
#define SOAPDELEGATOR_H_

#include "ProxyCertificateDelegator.h"

#include <delegation-cli/delegation-simple.h>

namespace fts3
{

namespace cli
{


class SoapDelegator : public fts3::cli::ProxyCertificateDelegator
{

public:

    SoapDelegator(std::string const & endpoint, std::string const & delegationId, long userRequestedDelegationExpTime) :
        ProxyCertificateDelegator(endpoint, delegationId, userRequestedDelegationExpTime),
        dctx (glite_delegation_new(endpoint.c_str()))
    {
        if (dctx == NULL)
            throw cli_exception("delegation: could not initialise a delegation context");
    }

    virtual ~SoapDelegator()
    {
        glite_delegation_free(dctx);
    }

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

    /// delegation context (a facade for the GSoap delegation client)
    glite_delegation_ctx *dctx;

};

} /* namespace cli */
} /* namespace fts3 */

#endif /* SOAPDELEGATOR_H_ */
