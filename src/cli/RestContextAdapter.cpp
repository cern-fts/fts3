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

std::vector< std::pair<std::string, std::string> > RestContextAdapter::cancel(std::vector<std::string> const & jobIds)
{
    std::vector<std::string>::const_iterator itr;

    std::vector< std::pair< std::string, std::string> > ret;

    for (itr = jobIds.begin(); itr != jobIds.end(); ++itr)
        {
            std::stringstream ss;
            std::string url = endpoint + "/jobs/" + *itr;
            HttpRequest http (url, capath, proxy, ss);
            http.del();

            ResponseParser p(ss);
            ret.push_back(std::make_pair(p.get("job_id"), p.get("job_state")));
        }

    return ret;
}

} /* namespace cli */
} /* namespace fts3 */
