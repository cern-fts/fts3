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

#ifndef BDIIBROWSER_H_
#define BDIIBROWSER_H_

#include <sys/types.h>
#include <signal.h>
#include "config/ServerConfig.h"

#include <ldap.h>

#include <sys/types.h>

#include <string>
#include <list>
#include <map>


#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include "common/Logger.h"
#include "common/Singleton.h"

namespace fts3
{
namespace infosys
{

using namespace common;

/**
 * BdiiBrowser class is responsible for browsing BDII.
 *
 * It has a singleton access, the BDII should be queried using 'browse' method.
 *
 * @see ThreadSafeInstanceHolder
 */
class BdiiBrowser: public Singleton<BdiiBrowser>
{

    friend class Singleton<BdiiBrowser>;

public:

    /// object class
    static const char* ATTR_OC;

    /// glue1 base
    static const std::string GLUE1;
    /// glue2 base
    static const std::string GLUE2;

    /// the object class for glue2
    static const char* CLASS_SERVICE_GLUE2;
    /// the object class for glue1
    static const char* CLASS_SERVICE_GLUE1;

    /// destructor
    virtual ~BdiiBrowser();

    /**
     * Checks if given VO is allowed for the given SE
     *
     * @param se - SE name
     * @param vo - VO name
     *
     * @return true if the VO is allowed, flase otherwise
     */
    bool isVoAllowed(std::string se, std::string vo);

    /**
     * Checks if the SE status allows for submitting a transfer
     *
     * @param se - SE name
     *
     * @return true if yes, otherwise false
     */
    bool getSeStatus(std::string se);

    // if we want all available attributes we leave attr = 0
    /**
     * Allows to browse the BDII
     *
     * Each time a browse action is taken the fts3config file is check for the right BDII endpoint
     *
     * @param base - either GLUE1 or GLUE2
     * @param query - the filter you want to apply
     * @param attr - the attributes you are querying for (by default 0, meaning you are interested in all attributes)
     *
     * @return the result set containing all the outcomes
     */
    template<typename R>
    std::list< std::map<std::string, R> > browse(std::string base, std::string query, const char **attr = 0);

    /**
     * Looks for attribute names in a foreign key
     *
     * @param values the values returned for the given foreign key
     * @param attr - the attributes you are querying for
     */
    static std::string parseForeingKey(std::list<std::string> values, const char *attr);

    /**
     * Checks in the fts3config if the given base (glue1 or glue2) is currently in use
     *
     * @param base - the base (glue1 or glue2) that will be checked
     *
     * @return true if the base is in use, false otherwise
     */
    bool checkIfInUse(const std::string& base);

private:

    /**
     * Connects to the BDII
     *
     * @param infosys - the BDII endpoint
     * @param sec - connection timeout in secounds (by default 15)
     *
     * @return true is connection was successful, otherwise false
     */
    bool connect(std::string infosys, time_t sec = 15);

    /**
     * Reconnect in case that the current connection is not valid any more
     *
     * @return true is it has been possible to reconnect successfully, false otherwise
     */
    bool reconnect();

    /**
     * Closes the current connection to the BDII
     */
    void disconnect();

    /**
     * Checks if the current connection is valid
     *
     * @return true if the curret connection is valid, false otherwise
     */
    bool isValid();

    /**
     * Converts the base string from BDII to ordinary string
     *
     * @param base - BDII base
     *
     * @return respective string value
     */
    std::string baseToStr(const std::string& base);

    /**
     * Parses BDII reply
     *
     * @param reply - the reply from BDII query
     *
     * @return result set containing the data retrived from BDII
     */
    template<typename R>
    std::list< std::map<std::string, R> > parseBdiiResponse(LDAPMessage *reply);

    /**
     * Parses single entry retrieved from BDII query
     *
     * @param entry - single entry returned from BDII query
     *
     * @return map that holds entry names and respective values
     */
    template<typename R>
    std::map<std::string, R> parseBdiiSingleEntry(LDAPMessage *entry);

    /**
     * Parses BDII attribute
     *
     * @param value - attribute value
     *
     * @return parsed value
     */
    template<typename R>
    R parseBdiiEntryAttribute(berval **value);


    /// LDAP context
    LDAP *ld;

    /// connection timeout
    timeval timeout;

    /// search timeout
    timeval search_timeout;

    /// full LDAP URL (including protocol)
    std::string url;

    /// the BDII endpoint
    std::string infosys;

    /**
     *  semaphore
     *  positive value gives the number of threads browsing BDII at the moment
     *  negative value gives the number of threads trying to reconnect
     */
    int querying;

    /// the mutex preventing concurrent browsing and reconnecting
    boost::shared_mutex qm;

    /// conditional variable preventing concurrent browsing and reconnecting
//    condition_variable qv;

    /**
     * Blocks until all threads finish browsing
     */
//    void waitIfBrowsing();

    /**
     * Notify all threads that want to do browsing after reconnection happened
     */
//    void notifyBrowsers();

    /**
     * Blocks until the reconnection is finished
     */
//    void waitIfReconnecting();

    /**
     * Notify all threads that want to perform reconnection after all the browsing is finished
     */
//    void notifyReconnector();

    /// not used for now
    static const char* ATTR_STATUS;
    /// not used for now
    static const char* ATTR_SE;

    /// not used for now
    static const std::string FIND_SE_STATUS(std::string se);
    /// not used for now
    static const char* FIND_SE_STATUS_ATTR[];

    /// a string 'false'
    static const std::string false_str;
    /// flag - true if connected, false if not
    bool connected;

    /// Constructor
    BdiiBrowser() : ld(NULL), querying(0), connected(false)
    {
        memset(&timeout, 0, sizeof(timeout));
        memset(&search_timeout, 0, sizeof(search_timeout));
    };
    /// not implemented
    BdiiBrowser(BdiiBrowser const&);
    /// not implemented
    BdiiBrowser& operator=(BdiiBrowser const&);

    /// maximum number of reconnection tries
    static const int max_reconnect = 3;

    ///@{
    /// keep alive LDAP parameters
    static const int keepalive_idle = 120;
    static const int keepalive_probes = 3;
    static const int keepalive_interval = 60;
    ///@}
};

template<typename R>
std::list< std::map<std::string, R> > BdiiBrowser::browse(std::string base, std::string query, const char **attr)
{
    signal(SIGPIPE, SIG_IGN);
    // check in the fts3config file if the 'base' (glue1 or glue2) is in use, if no return an empty result set
    if (!checkIfInUse(base))
        return std::list< std::map<std::string, R> >();

    // check in the fts3config if the host name for BDII was specified, if no return an empty result set
    if (!config::ServerConfig::instance().get<bool>("Infosys"))
        return std::list< std::map<std::string, R> >();

    // check if the connection is valid
    if (!isValid())
        {

            bool reconnected = false;
            int reconnect_count = 0;

            // try to reconnect 3 times
            for (reconnect_count = 0; reconnect_count < max_reconnect; reconnect_count++)
                {
                    reconnected = reconnect();
                    if (reconnected) break;
                }

            // if it has not been possible to reconnect return an empty result set
            if (!reconnected)
                {
                    FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: it has not been possible to reconnect to the BDII" << commit;
                    return std::list< std::map<std::string, R> >();
                }
        }

    int rc = 0;
    LDAPMessage *reply = 0;

    // used shared lock - many concurrent reads are allowed
    {
        boost::shared_lock<boost::shared_mutex> lock(qm);
        rc = ldap_search_ext_s(ld, base.c_str(), LDAP_SCOPE_SUBTREE, query.c_str(), const_cast<char**>(attr), 0, 0, 0, &timeout, 0, &reply);
    }

    if (rc != LDAP_SUCCESS)
        {
            if (reply && rc > 0) ldap_msgfree(reply);
            FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(rc) << commit;
            return std::list< std::map<std::string, R> > ();
        }

    std::list< std::map<std::string, R> > ret = parseBdiiResponse<R>(reply);
    if (reply)
        ldap_msgfree(reply);

    return ret;
}

template<typename R>
std::list< std::map<std::string, R> > BdiiBrowser::parseBdiiResponse(LDAPMessage *reply)
{
    std::list< std::map<std::string, R> > ret;
    for (LDAPMessage *entry = ldap_first_entry(ld, reply); entry != 0; entry = ldap_next_entry(ld, entry))
        {

            ret.push_back(
                parseBdiiSingleEntry<R>(entry)
            );
        }

    return ret;
}

template<typename R>
std::map<std::string, R> BdiiBrowser::parseBdiiSingleEntry(LDAPMessage *entry)
{

    BerElement *berptr = 0;
    char* attr = 0;
    std::map<std::string, R> m_entry;

    for (attr = ldap_first_attribute(ld, entry, &berptr); attr != 0; attr = ldap_next_attribute(ld, entry, berptr))
        {

            berval **value = ldap_get_values_len(ld, entry, attr);
            R val = parseBdiiEntryAttribute<R>(value);
            ldap_value_free_len(value);

            if (!val.empty())
                {
                    m_entry[attr] = val;
                }
            ldap_memfree(attr);
        }

    if (berptr) ber_free(berptr, 0);

    return m_entry;
}

template<>
inline std::string BdiiBrowser::parseBdiiEntryAttribute<std::string>(berval **value)
{

    if (value && value[0] && value[0]->bv_val)
	return value[0]->bv_val;
    return std::string();
}

template<>
inline std::list<std::string> BdiiBrowser::parseBdiiEntryAttribute< std::list<std::string> >(berval **value)
{

    std::list<std::string> ret;
    for (int i = 0; value && value[i] && value[i]->bv_val; i++)
        {
            ret.push_back(value[i]->bv_val);
        }
    return ret;
}

} /* namespace fts3 */
} /* namespace infosys */
#endif /* BDIIBROWSER_H_ */
