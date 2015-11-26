/*
 * RestModifyJob.cpp
 *
 * Created on: 13 Nov 2015
 * Author: dhsmith
 */

#include "RestModifyJob.h"

#include <utility>

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

namespace fts3
{
namespace cli
{

RestModifyJob::RestModifyJob(std::string jobId, int priority) : jobId(jobId) 
{
    // preparing the json to be sent in the body of the request.
    pt::ptree params;
    std::stringstream ss;
    ss << priority;
    params.push_back(std::make_pair("priority",pt::ptree(ss.str())));
    bodyjson.push_back(std::make_pair("params",params));
}

RestModifyJob::~RestModifyJob() { }

std::string RestModifyJob::body() const {
    std::stringstream str_out;
    pt::write_json(str_out, bodyjson);
    return str_out.str();
}

std::string RestModifyJob::resource() const {
    return "/jobs/" + jobId;
}

void RestModifyJob::do_http_action(HttpRequest &http) const {
    http.post();
}

} /* namespace cli */
} /* namespace fts3 */
