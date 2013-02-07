/*
 *	Copyright notice:
 *	Copyright � Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * BdiiBrowser.h
 *
 *  Created on: Oct 25, 2012
 *      Author: Michał SimonS
 */

#ifndef BDIIBROWSER_H_
#define BDIIBROWSER_H_

#include "common/logger.h"
#include "common/ThreadSafeInstanceHolder.h"

#include "config/serverconfig.h"

#include <ldap.h>

#include <sys/types.h>

#include <string>
#include <list>
#include <map>


#include <boost/thread.hpp>

namespace fts3 {
namespace infosys {

using namespace std;
using namespace boost;
using namespace common;
using namespace config;

class BdiiBrowser: public ThreadSafeInstanceHolder<BdiiBrowser> {

	friend class ThreadSafeInstanceHolder<BdiiBrowser>;

public:

	static const char* ATTR_OC;

	static const string GLUE1;
	static const string GLUE2;

	static const char* CLASS_SERVICE_GLUE2;
	static const char* CLASS_SERVICE_GLUE1;

	virtual ~BdiiBrowser();

	bool isVoAllowed(string se, string vo);
	bool getSeStatus(string se);

	// if we want all available attributes we leave attr = 0
	template<typename R>
	list< map<string, R> > browse(string base, string query, const char **attr = 0);

	static string parseForeingKey(list<string> values, const char *attr);

private:

	bool connect(string infosys = string(), time_t sec = 60);
	bool reconnect();
	void disconnect();
	bool isValid();

	template<typename R>
	list< map<string, R> > parseBdiiResponse(LDAPMessage *reply);

	template<typename R>
	map<string, R> parseBdiiSingleEntry(LDAPMessage *entry);

	template<typename R>
	R parseBdiiEntryAttribute(berval **value);



	LDAP *ld;
	timeval timeout;
	timeval search_timeout;
	string url;
	string infosys;

	int querying;
	mutex qm;
	condition_variable qv;

	void waitIfBrowsing();
	void notifyBrowsers();
	void waitIfReconnecting();
	void notifyReconnector();

	// not used for now
	static const char* ATTR_STATUS;
	static const char* ATTR_SE;

	static const string FIND_SE_STATUS(string se);
	static const char* FIND_SE_STATUS_ATTR[];

	static const string false_str;
	bool connected;

	BdiiBrowser() : querying(0), connected(false) {};
	BdiiBrowser(BdiiBrowser const&);
	BdiiBrowser& operator=(BdiiBrowser const&);

	static const int max_reconnect = 3;

	static const int max_reconnect2 = 3;

	static const int keepalive_idle = 120;
	static const int keepalive_probes = 3;
	static const int keepalive_interval = 60;
};

template<typename R>
list< map<string, R> > BdiiBrowser::browse(string base, string query, const char **attr) {

	// check in the config file if the BDII is in use, if not return an empty result set
	if (!theServerConfig().get<bool>("Infosys")) return list< map<string, R> >();

	// check if the connection is valied
	if (!isValid()) {

		bool reconnected = false;
		int reconnect_count = 0;

		// try to reconnect 3 times
		for (reconnect_count = 0; reconnect_count < max_reconnect; reconnect_count++) {
			reconnected = reconnect();
			if (reconnected) break;
		}

		// if it has not been possible to reconnect return an empty result set
		if (!reconnected) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: it has not been possible to reconnect to the BDII" << commit;
			return list< map<string, R> >();
		}
	}

    int rc = 0;
    LDAPMessage *reply = 0;

    waitIfReconnecting();
    rc = ldap_search_ext_s(ld, base.c_str(), LDAP_SCOPE_SUBTREE, query.c_str(), const_cast<char**>(attr), 0, 0, 0, &timeout, 0, &reply);
    notifyReconnector();

	if (rc != LDAP_SUCCESS) {
		if (reply && rc > 0) ldap_msgfree(reply);
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(rc) << commit;
    	return list< map<string, R> > ();
	}

	list< map<string, R> > ret = parseBdiiResponse<R>(reply);
	if (reply) ldap_msgfree(reply);

	return ret;
}

template<typename R>
list< map<string, R> > BdiiBrowser::parseBdiiResponse(LDAPMessage *reply) {

	list< map<string, R> > ret;
    for (LDAPMessage *entry = ldap_first_entry(ld, reply); entry != 0; entry = ldap_next_entry(ld, entry)) {

    	ret.push_back(
    				parseBdiiSingleEntry<R>(entry)
    			);
	}

	return ret;
}

template<typename R>
map<string, R> BdiiBrowser::parseBdiiSingleEntry(LDAPMessage *entry) {

	BerElement *berptr = 0;
	char* attr = 0;
	map<string, R> m_entry;

	for (attr = ldap_first_attribute(ld, entry, &berptr); attr != 0; attr = ldap_next_attribute(ld, entry, berptr)) {

		berval **value = ldap_get_values_len(ld, entry, attr);
		R val = parseBdiiEntryAttribute<R>(value);
		ldap_value_free_len(value);

		if (!val.empty()) {
			m_entry[attr] = val;
		}
    	ldap_memfree(attr);
	}

	if (berptr) ber_free(berptr, 0);

	return m_entry;
}

template<>
inline string BdiiBrowser::parseBdiiEntryAttribute<string>(berval **value) {

	if (value && value[0] && value[0]->bv_val) return value[0]->bv_val;
	return string();
}

template<>
inline list<string> BdiiBrowser::parseBdiiEntryAttribute< list<string> >(berval **value) {

	list<string> ret;
	for (int i = 0; value && value[i] && value[i]->bv_val; i++) {
		ret.push_back(value[i]->bv_val);
	}
	return ret;
}

} /* namespace fts3 */
} /* namespace infosys */
#endif /* BDIIBROWSER_H_ */
