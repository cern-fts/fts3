/*
 * SoapDelegator.cpp
 *
 *  Created on: 26 Aug 2014
 *      Author: simonm
 */

#include "SoapDelegator.h"

namespace fts3
{
namespace cli
{

boost::optional<time_t> SoapDelegator::getExpirationTime()
{
    // checking if there is already a delegated credential
    time_t expTime;
    int err = glite_delegation_info(dctx, delegationId.c_str(), &expTime);
    // if not return uninitialised optional
    if (err) return boost::none;
    // otherwise return the expiration time
    return expTime;
}

void SoapDelegator::doDelegation(time_t requestProxyDelegationTime, bool renew) const
{
    int err = glite_delegation_delegate(dctx, delegationId.c_str(), (int)(requestProxyDelegationTime/60), renew);

    if (err)
        throw cli_exception(glite_delegation_get_error(dctx));
}

} /* namespace cli */
} /* namespace fts3 */
