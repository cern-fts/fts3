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

#include "RestDelegator.h"

#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include <gridsite.h>
#include <globus/globus_gsi_system_config.h>

namespace fts3
{
namespace cli
{

boost::optional<time_t> RestDelegator::getExpirationTime()
{
    if (delegationId.empty())
        {
            std::string const whoami = endpoint + "/whoami";

            std::stringstream ss;
            HttpRequest http (whoami, capath, certkey, insecure, ss);
            http.get();

            ResponseParser parser(ss);
            delegationId = parser.get("delegation_id");
        }

    std::string const delegation = endpoint + "/delegation/" + delegationId;

    std::stringstream ss;
    HttpRequest http (delegation, capath, certkey, insecure, ss);
    http.get();

    if (ss.str() == "null") return boost::none;

    ResponseParser parser(ss);
    std::string resp = parser.get("termination_time");

    tm time;
    memset(&time, 0, sizeof(time));
    strptime(resp.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
    return timegm(&time);
}

void RestDelegator::doDelegation(time_t requestProxyDelegationTime, bool /*renew*/) const
{
    // do the request
    std::string const request = endpoint + "/delegation/" + delegationId + "/request";

    if (certkey.key.empty() || certkey.cert.empty()) {
        throw cli_exception("Unable to get user proxy filename!");
    }

    std::stringstream ss;
    HttpRequest(request, capath, certkey, insecure, ss).get();
    std::string certreq = ss.str();

    if (certreq.empty()) {
        throw cli_exception("The delegation request failed!");
    }

    char * certtxt;
    int ret = GRSTx509MakeProxyCert(
        &certtxt, stderr, (char*)certreq.c_str(),
        (char*)certkey.cert.c_str(), (char*)certkey.key.c_str(),
        (int)(requestProxyDelegationTime/60)
    );

    if (ret) throw cli_exception("GRSTx509MakeProxyCert call failed");

    // clear the stream
    ss.clear();
    ss.str("");

    // put the proxy
    std::string const put = endpoint + "/delegation/" + delegationId + "/credential";
    ss << certtxt;

    HttpRequest(put, capath, certkey, insecure, ss).put();
}

} /* namespace cli */
} /* namespace fts3 */
