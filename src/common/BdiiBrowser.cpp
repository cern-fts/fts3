/*
 * BdiiBrowser.cpp
 *
 *  Created on: Oct 25, 2012
 *      Author: simonm
 */

#include "BdiiBrowser.h"

#include <sys/types.h>
#include <sstream>

namespace fts3 {
namespace common {

const char* BdiiBrowser::ATTR_STATUS = "GlueSEStatus";
const char* BdiiBrowser::ATTR_OC = "objectClass";
const char* BdiiBrowser::ATTR_NAME = "GlueServiceUniqueID";
const char* BdiiBrowser::ATTR_URI = "GlueServiceURI";
const char* BdiiBrowser::ATTR_SE = "GlueSEUniqueID";
const char* BdiiBrowser::ATTR_LINK = "GlueForeignKey";
const char* BdiiBrowser::ATTR_SITE = "GlueSiteUniqueID";
const char* BdiiBrowser::ATTR_HOSTINGORG = "GlueServiceHostingOrganization";

#define CLASS_SERVICE		"GlueService"

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
//	ss << "(|(" << ATTR_NAME << "=*)";
//	ss << "(" << ATTR_URI << "=*))";
	ss << ")";
	return ss.str();
}
const char* BdiiBrowser::FIND_SE_SITE_ATTR[] = {BdiiBrowser::ATTR_LINK, BdiiBrowser::ATTR_HOSTINGORG, 0};

BdiiBrowser::BdiiBrowser(string url, long timeout) : url(url), timeout(timeout), base("o=grid") {

	connect();
}


BdiiBrowser::~BdiiBrowser() {

    disconnect();
}

void BdiiBrowser::connect() {
    int ret = 0;
    timeval t;

    ret = ldap_initialize(&ld, url.c_str());
    if (ret != LDAP_SUCCESS) {
        // TODO ldap_err2string(ret)
        return;
    }

    t.tv_sec = timeout;
    t.tv_usec = 0;
    ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout);

    berval cred;
    cred.bv_val = 0;
    cred.bv_len = 0;

    ret = ldap_sasl_bind_s(ld, 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);
    if (ret != LDAP_SUCCESS) {
        // TODO ldap_err2string(ret))
        return;
    }
}

void BdiiBrowser::disconnect() {

    if (ld) {
        ldap_unbind_ext_s(ld, 0, 0);
        ld = 0;
    }
}

void BdiiBrowser::reconnect() {

	disconnect();
	connect();
}

bool BdiiBrowser::isValid() {

	LDAPMessage *result = 0;
	int rc = ldap_search_ext_s(ld, "dc=example,dc=com", LDAP_SCOPE_BASE, "(sn=Curly)", 0, 0, 0, 0, 0, 0, &result);
	if (rc == LDAP_SUCCESS) {

		if (result) ldap_msgfree(result);
	    return true;

	} else if (rc == LDAP_SERVER_DOWN || rc == LDAP_CONNECT_ERROR) {

		if (result) ldap_msgfree(result);

	} else {
	    if (result) {
	    	ldap_msgfree(result);
	        return true;
	    }
	}

	return false;
}

template<typename R>
list< map<string, R> > BdiiBrowser::browse(string query, const char **attr, long timeout) {

	if (!isValid()) reconnect();

    timeval t;
    int rc = 0;

    t.tv_sec = timeout;
    t.tv_usec = 0;

    LDAPMessage *reply = 0;

    rc = ldap_search_ext_s(ld, base.c_str(), LDAP_SCOPE_SUBTREE, query.c_str(), const_cast<char**>(attr), 0, 0, 0, &t, 0, &reply);

	if (rc != LDAP_SUCCESS) {
		if (reply) ldap_msgfree(reply);
		// TODO ldap_err2string(ret)
		// if an exception will be thrown the 'rc == 0' check in for loop wont be needed
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
		return it->substr(pos + 1);
	}

	return string();
}

bool BdiiBrowser::getSeStatus(string se) {

	list< map<string, string> > rs = browse<string>(FIND_SE_STATUS(se), FIND_SE_STATUS_ATTR);

	if (rs.empty()) return true;

	string status = rs.front()[ATTR_STATUS];

	return status == "Production" || status == "Online";
}

string BdiiBrowser::getSiteName(string se) {

	map<string, string>::iterator it = seToSite.find(se);
	if (it != seToSite.end()) {
		return it->second;
	}

	list< map<string, list<string> > > rs = browse< list<string> >(FIND_SE_SITE(se), FIND_SE_SITE_ATTR);

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

} /* namespace ws */
} /* namespace fts3 */
