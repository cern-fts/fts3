/*
 * BdiiBrowser.cpp
 *
 *  Created on: Oct 25, 2012
 *      Author: simonm
 */

#include "BdiiBrowser.h"

#include <sys/types.h>

namespace fts3 {
namespace common {

const char* BdiiBrowser::ATTR_STATUS = "GlueSEStatus";
const char* BdiiBrowser::FIND_SE_STATUS_ATTR[] = {BdiiBrowser::ATTR_STATUS, 0};

const string BdiiBrowser::FIND_SE_STATUS(string se) {
	return "(&(GlueSEUniqueID=*" + se + "*))";
}

BdiiBrowser::BdiiBrowser(string url, long timeout) : url(url), timeout(timeout), base("o=grid"), attr(0) {

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
        char buf[256];
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

list< map<string, string> > BdiiBrowser::browse(string query, long timeout) {

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

	list< map<string, string> > ret;
    for (LDAPMessage *entry = ldap_first_entry(ld, reply); entry != 0 && rc == 0; entry = ldap_next_entry(ld, entry)) {

    	BerElement *berptr = 0;
    	char* attr = 0;
    	berval **value = 0;
    	map<string, string> m_entry;

    	for (attr = ldap_first_attribute(ld, entry, &berptr); attr != 0; attr = ldap_next_attribute(ld, entry, berptr)) {

    		value = ldap_get_values_len(ld, entry, attr);
			if (value) {
				if (value[0]->bv_val) m_entry[attr] = value[0]->bv_val;
				ber_bvecfree(value);
			}

        	ldap_memfree(attr);
    	}

    	if (berptr) ber_free(berptr, 0);
    	ret.push_back(m_entry);
	}

	if (reply) ldap_msgfree(reply);

	return ret;
}

bool BdiiBrowser::checkSeStatus(string se) {

	attr = FIND_SE_STATUS_ATTR; // if we want all available attributes we leave attr=0

	list< map<string, string> > rs = browse(FIND_SE_STATUS(se));

	if (rs.empty()) return true;

	string status = rs.front()[ATTR_STATUS];

	return status == "Production" || status == "Online";
}

} /* namespace ws */
} /* namespace fts3 */
