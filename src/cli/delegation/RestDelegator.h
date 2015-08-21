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
