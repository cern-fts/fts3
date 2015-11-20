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

#include <boost/regex.hpp>
#include "Uri.h"

namespace fts3 {
namespace common {


// From http://www.ietf.org/rfc/rfc2396.txt
// Appendix B
#define URI_REGEX "^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?"

static boost::regex uri_regex(URI_REGEX);

static void extractPort(Uri& u0)
{
    size_t bracket_close_i = u0.host.rfind(']'); // Account for IPv6 in the host name
    size_t colon_i = u0.host.rfind(':');

    // No port
    if (colon_i == std::string::npos)
        return;

    // IPv6 without port
    if (bracket_close_i != std::string::npos && bracket_close_i > colon_i)
        return;

    std::string port_str = u0.host.substr(colon_i + 1);
    u0.host = u0.host.substr(0, colon_i);
    u0.port = atoi(port_str.c_str());
}


    Uri Uri::parse(const std::string &uri)
    {
        Uri u0;

        boost::smatch matches;
        if (boost::regex_match(uri, matches, uri_regex, boost::match_posix)) {
            u0.protocol = matches[2];
            u0.host = matches[4];
            u0.path = matches[5];
            u0.queryString = matches[7];

            // port is put into host, so extract it
            u0.port = 0;
            extractPort(u0);
        }

        return u0;
    }

} // namespace common
} // namespace fts3
