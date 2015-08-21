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

    SoapDelegator(std::string const & endpoint, std::string const & delegationId, long userRequestedDelegationExpTime, std::string const& proxy) :
        ProxyCertificateDelegator(endpoint, delegationId, userRequestedDelegationExpTime, proxy)
    {
        dctx = glite_delegation_new(endpoint.c_str(), proxy.c_str());
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
