/*
 * RestContextAdapter.cpp
 *
 *  Created on: 15 Aug 2014
 *      Author: simonm
 */

#include "RestContextAdapter.h"

#include "rest/HttpRequest.h"
#include "rest/ResponseParser.h"

#include <iostream>
#include <sstream>

namespace fts3 {
namespace cli {

void RestContextAdapter::getInterfaceDeatailes()
{
    std::stringstream ss;
    HttpRequest http (endpoint, capath, proxy, ss);
    http.get();

    ResponseParser parser(ss);

    version += parser.get("api.major");
    version += "." + parser.get("api.minor");
    version += "." + parser.get("api.patch");

    interface = version;
    metadata  = "fts3-rest-" + version;

    schema += parser.get("schema.major");
    schema += "." + parser.get("schema.minor");
    schema += "." + parser.get("schema.patch");
}

std::vector<JobStatus> RestContextAdapter::listRequests (std::vector<std::string> const & statuses, std::string const & dn, std::string const & vo, std::string const & /*source*/, std::string const & /*destination*/)
{
    // prefix will be holding '?' at the first concatenation and then '&'
    char prefix = '?';
    std::string url = endpoint + "/jobs";

    if (!dn.empty())
        {
            url += prefix;
            url += "user_dn=";
            url += dn;
            prefix = '&';
        }

    if (!vo.empty())
        {
            url += prefix;
            url += "vo_name=";
            url += vo;
            prefix = '&';
        }

    if (!statuses.empty())
        {
            url += prefix;
            url += "job_state=";
            url += *statuses.begin();
            prefix = '&';
        }

    std::stringstream ss;
    ss << "{\"jobs\":";
    HttpRequest http (url, capath, proxy, ss);
    http.get();
    ss << '}';

    ResponseParser parser(ss);
    return parser.getJobs("jobs");
}

} /* namespace cli */
} /* namespace fts3 */
