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
 * BdiiBrowser.cpp
 *
 *  Created on: Oct 25, 2012
 *      Author: Michał Simon
 */

#include "BdiiBrowser.h"
#include "logger.h"

#include <sstream>

namespace fts3 {
namespace common {

// TODO reorganize + do queries for glue2

const string BdiiBrowser::GLUE1 = "o=grid";
const string BdiiBrowser::GLUE2 = "o=glue";

const char* BdiiBrowser::ATTR_STATUS = "GlueSEStatus";
const char* BdiiBrowser::ATTR_OC = "objectClass";
const char* BdiiBrowser::ATTR_NAME = "GlueServiceUniqueID";
const char* BdiiBrowser::ATTR_URI = "GlueServiceURI";
const char* BdiiBrowser::ATTR_SE = "GlueSEUniqueID";
const char* BdiiBrowser::ATTR_LINK = "GlueForeignKey";
const char* BdiiBrowser::ATTR_SITE = "GlueSiteUniqueID";
const char* BdiiBrowser::ATTR_HOSTINGORG = "GlueServiceHostingOrganization";

const char* BdiiBrowser::CLASS_SERVICE = "GlueService";

const string BdiiBrowser::false_str = "false";

/* Query expressing "No VO attribute" */
#define QUERY_VO_ANY "(!(" ATTR_VO "=*))"

#define QUERY_VO_PRE  "(|" QUERY_VO_ANY
#define QUERY_VO      "(" ATTR_VO "=%s)"
#define QUERY_VO_POST ")"

static int keepalive_idle = 120;
static int keepalive_probes = 3;
static int keepalive_interval = 60;

// "(| (%sAccessControlBaseRule=VO:%s) (%sAccessControlBaseRule=%s) (%sAccessControlRule=%s)"

const string BdiiBrowser::FIND_SE_STATUS(string se) {

	stringstream ss;
	ss << "(&(" << BdiiBrowser::ATTR_SE << "=*" << se << "*))";
	return ss.str();
}
const char* BdiiBrowser::FIND_SE_STATUS_ATTR[] = {BdiiBrowser::ATTR_STATUS, 0};

const string BdiiBrowser::FIND_SE_SITE(string se) {
	stringstream ss;
	ss << "(&";
	ss << "(" << BdiiBrowser::ATTR_OC << "=" << CLASS_SERVICE << ")";
	ss << "(|(" << ATTR_NAME << "=*" << se << "*)";
	ss << "(" << ATTR_URI << "=*" << se << "*))";
	ss << ")";
	return ss.str();
}
const char* BdiiBrowser::FIND_SE_SITE_ATTR[] = {BdiiBrowser::ATTR_LINK, BdiiBrowser::ATTR_HOSTINGORG, 0};


BdiiBrowser::~BdiiBrowser() {

    disconnect();
}

bool BdiiBrowser::connect(string infosys, time_t sec) {

	this->infosys = infosys;
	inuse = infosys != false_str;
	if (!inuse) return false;

	timeout.tv_sec = sec;
	timeout.tv_usec = 0;
	
	search_timeout.tv_sec = (sec*2);
	search_timeout.tv_usec = 0;	

	url = "ldap://" + infosys;

	int ret = 0;
    ret = ldap_initialize(&ld, url.c_str());
    if (ret != LDAP_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(ret) << commit;
    	return false;
    }

    ldap_set_option(ld, LDAP_OPT_TIMEOUT, &search_timeout);
    ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);
    ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_IDLE,(void *) &(keepalive_idle));
    ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_PROBES,(void *) &(keepalive_probes));
    ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_INTERVAL, (void *) &(keepalive_interval));       

    berval cred;
    cred.bv_val = 0;
    cred.bv_len = 0;

    ret = ldap_sasl_bind_s(ld, 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);
    if (ret != LDAP_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(ret) << commit;
    	return false;
    }
    
    return true;
}

void BdiiBrowser::disconnect() {

    if (ld) {
        ldap_unbind_ext_s(ld, 0, 0);
        ld = 0;
    }
}

void BdiiBrowser::waitIfBrowsing() {
	mutex::scoped_lock lock(qm);
	while (querying != 0) qv.wait(lock);
	--querying;
}

void BdiiBrowser::notifyBrowsers() {
	mutex::scoped_lock lock(qm);
	++querying;
	qv.notify_all();
}

void BdiiBrowser::waitIfReconnecting() {
	mutex::scoped_lock lock(qm);
	while (querying < 0) qv.wait(lock);
	++querying;
}

void BdiiBrowser::notifyReconnector() {
	mutex::scoped_lock lock(qm);
	--querying;
	qv.notify_one();
}

void BdiiBrowser::reconnect() {

	waitIfBrowsing();

	disconnect();
	connect(infosys, timeout.tv_sec);

	notifyBrowsers();
}

bool BdiiBrowser::isValid() {	
	LDAPMessage *result = 0;

	waitIfReconnecting();
	int rc = ldap_search_ext_s(ld, "dc=example,dc=com", LDAP_SCOPE_BASE, "(sn=Curly)", 0, 0, 0, 0, &timeout, 0, &result);
	notifyReconnector();

	if (rc == LDAP_SUCCESS) {

		if (result) ldap_msgfree(result);
	    return true;

	} else if (rc == LDAP_SERVER_DOWN || rc == LDAP_CONNECT_ERROR) {

		if (result) ldap_msgfree(result);

	} else {
		// we only free the memory if rc > 0 because of a bug in thread-safe version of LDAP lib
	    if (result && rc > 0) {
	    	ldap_msgfree(result);
	    }

	    return true;
	}

	return false;
}

template<typename R>
list< map<string, R> > BdiiBrowser::browse(string base, string query, const char **attr) {

	if (!inuse) return list< map<string, R> >();

	if (!isValid()) reconnect();

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

string BdiiBrowser::parseForeingKey(list<string> values, const char *attr) {

	list<string>::iterator it;
	for (it = values.begin(); it != values.end(); it++) {
		size_t pos = it->find('=');
		if (it->substr(0, pos) == attr) return it->substr(pos + 1);
	}

	return string();
}

bool BdiiBrowser::getSeStatus(string se) {

	list< map<string, string> > rs = browse<string>(GLUE1, FIND_SE_STATUS(se), FIND_SE_STATUS_ATTR);

	if (rs.empty()) return true;

	string status = rs.front()[ATTR_STATUS];

	return status == "Production" || status == "Online";
}

string BdiiBrowser::getSiteName(string se) {

	map<string, string>::iterator it = seToSite.find(se);
	if (it != seToSite.end()) {
		return it->second;
	}
	list< map<string, list<string> > > rs = browse< list<string> >(GLUE1, FIND_SE_SITE(se), FIND_SE_SITE_ATTR);

	if (rs.empty()) return string();

	list<string> values = rs.front()[ATTR_LINK];
	string site = parseForeingKey(values, ATTR_SITE);

	if (site.empty()) {
		site = rs.front()[ATTR_HOSTINGORG].front();
	}

	seToSite[se] = site;
	if(seToSite.size() > 5000) seToSite.clear();

	return site;
}

bool BdiiBrowser::isVoAllowed(string se, string vo) {
	se = std::string();
	vo = std::string();	
	return true;
}

template<>
string BdiiBrowser::parseBdiiEntryAttribute<string>(berval **value) {

	if (value && value[0] && value[0]->bv_val) return value[0]->bv_val;
	return string();
}

template<>
list<string> BdiiBrowser::parseBdiiEntryAttribute< list<string> >(berval **value) {

	list<string> ret;
	for (int i = 0; value && value[i] && value[i]->bv_val; i++) {
		ret.push_back(value[i]->bv_val);
	}
	return ret;
}

} /* namespace common */
} /* namespace fts3 */
