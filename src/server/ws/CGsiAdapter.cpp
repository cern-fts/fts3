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

#include "CGsiAdapter.h"

#include <cgsi_plugin.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "common/error.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fts3
{
namespace ws
{

using namespace fts3::common;
using namespace boost;


const string CGsiAdapter::hostDn = CGsiAdapter::initHostDn();

// Build a 'virtual vo' from the user DN, as follows:
// Last CN without spaces @ concatenations of DC with dots
// i.e.
// Input: /DC=ch/DC=cern/OU=Organic Units/.../CN=jdoe/CN=1234/CN=Jon Doe
// Output: JonDoe@cern.ch
static std::string vo_from_user_dn(const std::string& dn)
{
    std::string user, domain;

    std::vector<std::string> components;
    boost::split(components, dn, boost::is_any_of("/"));
    for (std::vector<std::string>::const_iterator c = components.begin(); c != components.end(); ++c)
        {
            if (!c->empty())
                {
                    std::vector<std::string> pair;
                    boost::split(pair, *c, boost::is_any_of("="));
                    if (pair.size() == 2)
                        {
                            if (pair[0] == "CN")
                                {
                                    user = pair[1];
                                }
                            else if (pair[0] == "DC")
                                {
                                    if (domain.empty())
                                        domain = pair[1];
                                    else
                                        domain = pair[1] + '.' + domain;
                                }
                        }
                }
        }

    boost::erase_all(user, " ");
    if (user.empty() || domain.empty())
        return "nil";
    else
        return user + '@' + domain;
}


CGsiAdapter::CGsiAdapter(soap* ctx) : ctx(ctx)
{
    // get client DN
    const int len = 200;
    char buff[len] = {0};
    if (get_client_dn(ctx, buff, len)) throw Err_Custom("'get_client_dn' failed!");
    dn = buff;

    // get client VO
    char* tmp = get_client_voname(ctx);
    if(tmp) vo = tmp;
    else vo = vo_from_user_dn(dn);

    // retrieve VOMS attributes (fqnas)
    int nbfqans = 0;
    char **arr = get_client_roles(ctx, &nbfqans);

    for (int i = 0; i < nbfqans; i++)
        {
            attrs.push_back(arr[i]);
        }
}

CGsiAdapter::~CGsiAdapter()
{
}

string CGsiAdapter::getClientVo()
{
    return vo;
}

string CGsiAdapter::getClientDn()
{
    return dn;
}

vector<string> CGsiAdapter::getClientAttributes()
{
    return attrs;
}

vector<string> CGsiAdapter::getClientRoles()
{

    static const regex re ("/.*/Role=(\\w+)/.*");
    static const int ROLE_VALUE = 1;

    vector<string> ret;
    vector<string>::iterator it;

    for (it = attrs.begin(); it != attrs.end(); ++it)
        {

            smatch what;
            regex_match(*it, what, re, match_extra);
            ret.push_back(what[ROLE_VALUE]);
        }

    return ret;
}

string CGsiAdapter::initHostDn()
{
    const string hostCert = "/etc/grid-security/fts3hostcert.pem";
    string dn;

    struct stat buffer;
    if (stat(hostCert.c_str(), &buffer) != 0)
        return std::string("");

    // check the server host certificate
    FILE *fp = fopen(hostCert.c_str(), "r");
    X509 *cert = NULL;
    if (fp)
        {
            cert = PEM_read_X509(fp, 0, 0, 0);
            fclose(fp);
        }

    if (!cert)
        return string();

    dn = cert->name;
    X509_free(cert);

    return dn;
}

} /* namespace ws */
} /* namespace fts3 */
