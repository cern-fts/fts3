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

#include "Uri.h"

#include <glib.h>
#include <netdb.h>
#include <sys/param.h>
#include <unistd.h>
#include <boost/regex.hpp>


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
    u0.fullUri = uri;

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


bool isLanTransfer(const std::string &source, const std::string &dest)
{
    if (source == dest)
        return true;

    std::string sourceDomain;
    std::string destinDomain;

    std::size_t foundSource = source.find(".");
    std::size_t foundDestin = dest.find(".");

    if (foundSource != std::string::npos) {
        sourceDomain = source.substr(foundSource, source.length());
    }

    if (foundDestin != std::string::npos) {
        destinDomain = dest.substr(foundDestin, dest.length());
    }

    if (sourceDomain == destinDomain)
        return true;

    return false;
}


std::string getFullHostname()
{
    char hostname[MAXHOSTNAMELEN] = {0};
    gethostname(hostname, sizeof(hostname));

    struct addrinfo hints, *info;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    // First is OK
    if (getaddrinfo(hostname, NULL, &hints, &info) == 0) {
        g_strlcpy(hostname, info->ai_canonname, sizeof(hostname));
        freeaddrinfo(info);
    }
    return hostname;
}


} // namespace common
} // namespace fts3
