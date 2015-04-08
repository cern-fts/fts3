/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ProxyCertificateDelegator.h
 *
 *  Created on: Jun 4, 2012
 *      Author: simonm
 */

#ifndef ProxyCertificateDelegator_H_
#define ProxyCertificateDelegator_H_

#include "MsgPrinter.h"

#include <string>

#include <boost/optional.hpp>

namespace fts3
{
namespace cli
{

/**
 * The ProxyCertificateDelegator handles proxy certificate delegation
 *
 * If there is no proxy certificate yet it is delegated, if there is one
 * already and it expires in less than 4 hours it will be renewed
 *
 */
class ProxyCertificateDelegator
{
public:

    /**
     * Constructor
     *
     * Creates the delegation context.
     *
     * @param endpoint - the FTS3 service endpoint
     * @param delegationId - delegation ID (empty std::string means that the default ID should be used)
     * @param userRequestedDelegationExpTime - user specified delegation expire time
     * 											(0 means that the default value should be used)
     * @param proxy - The proxy path
     */
    ProxyCertificateDelegator(std::string const & endpoint,
            std::string const & delegationId,
            long userRequestedDelegationExpTime, std::string const & proxy) :
        delegationId(delegationId),
        endpoint(endpoint),
        proxy(proxy),
        userRequestedDelegationExpTime(userRequestedDelegationExpTime)
    {

    }

    /**
     * Destructor
     *
     * Deallocates the delegation context
     */
    virtual ~ProxyCertificateDelegator() {}

    /**
     * Delegates the proxy certificate
     *
     * If there is no proxy certificate on the server side the proxy is delegated.
     * If the proxy certificate on the server side will expire in less than 4 hours
     * the proxy certificate is redelegated.
     * Otherwise no action is taken. Moreover, a proxy is never delegated if its
     * expiration date is shorter than of the proxy certificate on the server side
     *
     * @return true if the operation was successful, false otherwise
     */
    void delegate();

    /**
     * Checks the expiration date of the local proxy certificate
     * @return expiration date of the proxy certificate
     */
    long isCertValid() const;

    static const int REDELEGATION_TIME_LIMIT = 3600*6;
    static const int MAXIMUM_TIME_FOR_DELEGATION_REQUEST = 3600 * 24;

protected:
    /// delegation ID
    std::string delegationId;
    /// FTS3 service endpoint
    std::string const endpoint;
    // proxy path
    std::string proxy;
    /// user defined proxy certificate expiration time
    long const userRequestedDelegationExpTime;

private:

    /// @return : expiration time of the credential on server, or uninitialised optional if credential does not exist
    virtual boost::optional<time_t> getExpirationTime() = 0;

    /**
     * Does the delegation (either using soap or rest, implementation specific)
     *
     * @param requestProxyDelegationTime : expiration time requested for the proxy
     * @param renew : renew flag
     */
    virtual void doDelegation(time_t requestProxyDelegationTime, bool renew) const = 0;
};

}
}

#endif /* ProxyCertificateDelegator_H_ */
