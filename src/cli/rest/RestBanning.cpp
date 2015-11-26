/*
 * RestBanning.cpp
 *
 * Created on: 10 Nov 2015
 * Author: dhsmith
 */

#include "RestBanning.h"

#include <utility>

#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp>

namespace fts3
{
namespace cli
{

RestBanning::RestBanning(std::string name, std::string vo, std::string status, int timeout, bool mode, bool isuser) : name(name), mode(mode), isUser(isuser)
{
    // preparing the json to be sent in the body of requests.
    // no json needed in the request body for unbanning case
    if (!mode)
        return;

    if (isUser) {
        bodyjson.push_back(std::make_pair("user_dn", pt::ptree(name)));
    } else {
        bodyjson.push_back(std::make_pair("storage",pt::ptree(name)));
    }

    // nothing more needed for unban case
    if (isUser)
        return;

    bool allow_submit = false;
    if (status == "WAIT_AS") {
        status = "WAIT";
        allow_submit = true;
    }

    // for banning for se case
    bodyjson.push_back(std::make_pair("vo_name",pt::ptree(vo)));
    if (allow_submit) {
        bodyjson.push_back(std::make_pair("allow_submit",pt::ptree("1")));
    }
    bodyjson.push_back(std::make_pair("status",pt::ptree(status)));
    std::stringstream ss;
    ss << timeout;
    bodyjson.push_back(std::make_pair("timeout",pt::ptree(ss.str())));
}

RestBanning::~RestBanning() { }

std::string RestBanning::body() const {
    if (!mode) {
        // nothing is sent for del requests
        return std::string();
    }
    std::stringstream str_out;
    pt::write_json(str_out, bodyjson);
    return str_out.str();
}

std::string RestBanning::resource() const {
    std::string ret;

    ret = isUser ? "/ban/dn" : "/ban/se";

    if (!mode) {
        ret += isUser ? "?user_dn=" : "?storage=";
        ret += HttpRequest::urlencode(name);
    }

    return ret;
}

void RestBanning::do_http_action(HttpRequest &http) const {
    if (mode) {
        http.post();
    } else {
        http.del();
    }
}

} /* namespace cli */
} /* namespace fts3 */
