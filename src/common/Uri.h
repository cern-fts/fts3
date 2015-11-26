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

#pragma once
#ifndef URI_H_
#define URI_H_

#include <string>

namespace fts3 {
namespace common {

/// Hold a Uri, splitted in relevant parts
class Uri
{
public:
    std::string queryString, path, protocol, host;
    unsigned port;

    Uri(): port(0) {}

    std::string getSeName(void)
    {
        return protocol + "://" + host;
    }

    static Uri parse(const std::string &uri);
};

/// Returns true if source and destination hosts are considered to belong
/// to the same LAN
bool isLanTransfer(const std::string &source, const std::string &dest);

/// Return the full qualified hostname for this host
std::string getFullHostname();


} // namespace common
} // namespace fts3

#endif // URI_H_
