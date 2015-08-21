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

#include "BdiiBrowser.h"

#include <sstream>

namespace fts3
{
namespace infosys
{

const string BdiiBrowser::GLUE1 = "o=grid";
const string BdiiBrowser::GLUE2 = "o=glue";

const char* BdiiBrowser::ATTR_STATUS = "GlueSEStatus";
const char* BdiiBrowser::ATTR_SE = "GlueSEUniqueID";

// common for both GLUE1 and GLUE2
const char* BdiiBrowser::ATTR_OC = "objectClass";

const char* BdiiBrowser::CLASS_SERVICE_GLUE2 = "GLUE2Service";
const char* BdiiBrowser::CLASS_SERVICE_GLUE1 = "GlueService";

const string BdiiBrowser::false_str = "false";

// "(| (%sAccessControlBaseRule=VO:%s) (%sAccessControlBaseRule=%s) (%sAccessControlRule=%s)"

const string BdiiBrowser::FIND_SE_STATUS(string se)
{

    stringstream ss;
    ss << "(&(" << BdiiBrowser::ATTR_SE << "=*" << se << "*))";
    return ss.str();
}
const char* BdiiBrowser::FIND_SE_STATUS_ATTR[] = {BdiiBrowser::ATTR_STATUS, 0};


BdiiBrowser::~BdiiBrowser()
{

    disconnect();
}

bool BdiiBrowser::connect(string infosys, time_t sec)
{

    // make sure that the infosys string is not 'false'
    if (infosys == false_str) return false;

    this->infosys = infosys;

    timeout.tv_sec = sec;
    timeout.tv_usec = 0;

    search_timeout.tv_sec = sec * 2;
    search_timeout.tv_usec = 0;

    url = "ldap://" + infosys;

    int ret = 0;
    ret = ldap_initialize(&ld, url.c_str());
    if (ret != LDAP_SUCCESS)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error init: " << ldap_err2string(ret) << " " << infosys << commit;
            disconnect();
            return false;
        }

    if (ldap_set_option(ld, LDAP_OPT_TIMEOUT, &search_timeout) != LDAP_OPT_SUCCESS)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_TIMEOUT): " << ldap_err2string(ret) << " " << infosys << commit;
        }

    if (ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout) != LDAP_OPT_SUCCESS)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_NETWORK_TIMEOUT): " << ldap_err2string(ret) << " " << infosys << commit;
        }

    // set the keep alive if it has been set to 'true' in the fts3config file
    if (theServerConfig().get<bool>("BDIIKeepAlive"))
        {

            int val = keepalive_idle;
            if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_IDLE,(void *) &val) != LDAP_OPT_SUCCESS)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_IDLE): " << ldap_err2string(ret) << " " << infosys << commit;
                }

            val = keepalive_probes;
            if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_PROBES,(void *) &val) != LDAP_OPT_SUCCESS)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_PROBES): " << ldap_err2string(ret) << " " << infosys << commit;
                }

            val = keepalive_interval;
            if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_INTERVAL, (void *) &val) != LDAP_OPT_SUCCESS)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_INTERVAL): " << ldap_err2string(ret) << " " << infosys << commit;
                }
        }

    berval cred;
    cred.bv_val = 0;
    cred.bv_len = 0;

    ret = ldap_sasl_bind_s(ld, 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);
    if (ret != LDAP_SUCCESS)
        {
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error bind: " << ldap_err2string(ret) << " " << infosys << commit;
            disconnect();
            return false;
        }

    connected = true;

    return true;
}

void BdiiBrowser::disconnect()
{

    if (ld)
        {
            ldap_unbind_ext_s(ld, 0, 0);
            ld = 0;
        }

    connected = false;
}

bool BdiiBrowser::reconnect()
{
    signal(SIGPIPE, SIG_IGN);

    // use unique locking - while reconnecting noone else can acquire the mutex
    unique_lock<shared_mutex> lock(qm);

    if (connected) disconnect();
    bool ret = connect(theServerConfig().get<string>("Infosys"));

    return ret;
}

bool BdiiBrowser::isValid()
{

    // if we are not connected the connection is not valid
    if (!connected) return false;
    // if the bdii host have changed it is also not valid
    if (theServerConfig().get<string>("Infosys") != this->infosys) return false;

    LDAPMessage *result = 0;
    signal(SIGPIPE, SIG_IGN);
    int rc = 0;
    // used shared lock - many concurrent reads are allowed
    {
        shared_lock<shared_mutex> lock(qm);
        rc = ldap_search_ext_s(ld, "dc=example,dc=com", LDAP_SCOPE_BASE, "(sn=Curly)", 0, 0, 0, 0, &timeout, 0, &result);
    }

    if (rc == LDAP_SUCCESS)
        {

            if (result) ldap_msgfree(result);
            return true;

        }
    else if (rc == LDAP_SERVER_DOWN || rc == LDAP_CONNECT_ERROR)
        {

            if (result) ldap_msgfree(result);

        }
    else
        {
            // we only free the memory if rc > 0 because of a bug in thread-safe version of LDAP lib
            if (result && rc > 0)
                {
                    ldap_msgfree(result);
                }

            return true;
        }

    return false;
}

string BdiiBrowser::parseForeingKey(list<string> values, const char *attr)
{

    list<string>::iterator it;
    for (it = values.begin(); it != values.end(); ++it)
        {

            string entry = *it, attr_str = attr;
            to_lower(entry);
            to_lower(attr_str);

            size_t pos = entry.find('=');
            if (entry.substr(0, pos) == attr_str) return it->substr(pos + 1);
        }

    return string();
}

bool BdiiBrowser::getSeStatus(string se)
{

    list< map<string, string> > rs = browse<string>(GLUE1, FIND_SE_STATUS(se), FIND_SE_STATUS_ATTR);

    if (rs.empty()) return true;

    string status = rs.front()[ATTR_STATUS];

    return status == "Production" || status == "Online";
}

bool BdiiBrowser::isVoAllowed(string se, string vo)
{
    se = std::string();
    vo = std::string();
    return true;
}

string BdiiBrowser::baseToStr(const string& base)
{

    if (base == GLUE1) return "glue1";
    if (base == GLUE2) return "glue2";

    return string();
}

bool BdiiBrowser::checkIfInUse(const string& base)
{

    string base_str = baseToStr(base);

    vector<string> providers = theServerConfig().get< vector<string> >("InfoProviders");
    vector<string>::iterator it;

    for (it = providers.begin(); it != providers.end(); ++it)
        {
            if (base_str == *it) return true;
        }

    return false;
}

} /* namespace common */
} /* namespace fts3 */
