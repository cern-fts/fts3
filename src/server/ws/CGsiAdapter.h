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

#ifndef CGSIADAPTER_H_
#define CGSIADAPTER_H_

//#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <stdsoap2.h>
#include <string>
#include <vector>

namespace fts3
{
namespace ws
{


class CGsiAdapter
{

public:
    CGsiAdapter(soap* ctx);
    virtual ~CGsiAdapter();

    /**
     * Gets the VO name of the client
     *
     * @return VO name
     */
    std::string getClientVo();

    /**
     * Gets the DN of the client
     *
     * @return client DN
     */
    std::string getClientDn();

    /**
     * Gets the voms attributes of the client
     *
     * @return vector with client's voms attributes
     */
    std::vector<std::string> getClientAttributes();

    /**
     * Gets the roles of the client
     *
     * @return vector with client's roles
     */
    std::vector<std::string> getClientRoles();

    /**
     * Checks if the client was a root user of the server hosting fts3
     * 	(it is assumed that the client is the root user of the server
     * 	hosting fts3 if he uses the server certificate to authenticate
     * 	himself)
     *
     * @return true if the client is a root user, false otherwise
     */
    bool isRoot()
    {
        if (hostDn.empty()) return false;
        return dn == hostDn;
    }

private:
    /// GSoap context
    soap* ctx;


    /// client VO
    std::string vo;

    /// client DN
    std::string dn;

    /// client roles
    std::vector<std::string> attrs;

    /// checks the host DN
    static std::string initHostDn();

    /// host dn
    static const std::string hostDn;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* CGSIADAPTER_H_ */
