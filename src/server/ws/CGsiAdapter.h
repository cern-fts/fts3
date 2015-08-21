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

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <string>
#include <vector>

namespace fts3
{
namespace ws
{

using namespace std;

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
    string getClientVo();

    /**
     * Gets the DN of the client
     *
     * @return client DN
     */
    string getClientDn();

    /**
     * Gets the voms attributes of the client
     *
     * @return vector with client's voms attributes
     */
    vector<string> getClientAttributes();

    /**
     * Gets the roles of the client
     *
     * @return vector with client's roles
     */
    vector<string> getClientRoles();

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
    string vo;

    /// client DN
    string dn;

    /// client roles
    vector<string> attrs;

    /// checks the host DN
    static string initHostDn();

    /// host dn
    static const string hostDn;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* CGSIADAPTER_H_ */
