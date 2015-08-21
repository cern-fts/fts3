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
