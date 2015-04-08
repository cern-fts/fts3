/*
 * RestDelegator.cpp
 *
 *  Created on: 26 Aug 2014
 *      Author: simonm
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
            HttpRequest http (whoami, capath, proxy, ss);
            http.get();

            ResponseParser parser(ss);
            delegationId = parser.get("delegation_id");
        }

    std::string const delegation = endpoint + "/delegation/" + delegationId;

    std::stringstream ss;
    HttpRequest http (delegation, capath, proxy, ss);
    http.get();

    if (ss.str() == "null") return boost::none;

    ResponseParser parser(ss);
    std::string resp = parser.get("termination_time");

    tm time;
    strptime(resp.c_str(), "%Y-%m-%dT%H:%M:%S", &time);
    return mktime(&time);
}

void RestDelegator::doDelegation(time_t requestProxyDelegationTime, bool renew) const
{
    // do the request
    std::string const request = endpoint + "/delegation/" + delegationId + "/request";

    char * localproxy = NULL;
    if (!proxy.empty())
        {
            localproxy = (char*)proxy.c_str();
        }
    else if (NULL == (localproxy = getenv("X509_USER_PROXY")))
        {
            if (GLOBUS_GSI_SYSCONFIG_GET_PROXY_FILENAME(&localproxy, GLOBUS_PROXY_FILE_INPUT))
                {
                    throw cli_exception("Unable to get user proxy filename!");
                }
        }

    std::stringstream ss;
    HttpRequest(request, capath, proxy, ss).get();
    std::string certreq = ss.str();

    if (certreq.empty()) throw cli_exception("The delegation request failed!");

    char * certtxt;
    int ret = GRSTx509MakeProxyCert(
                  &certtxt, stderr, (char*)certreq.c_str(), localproxy, localproxy, (int)(requestProxyDelegationTime/60)
              );

    if (ret) throw cli_exception("GRSTx509MakeProxyCert call failed");

    // clear the stream
    ss.clear();
    ss.str("");

    // put the proxy
    std::string const put = endpoint + "/delegation/" + delegationId + "/credential";
    ss << certtxt;

    HttpRequest(put, capath, proxy, ss).put();
}

} /* namespace cli */
} /* namespace fts3 */
