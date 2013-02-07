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

#include "common/logger.h"

#include "config/serverconfig.h"

#include <sstream>

namespace fts3 {
namespace infosys {

using namespace config;

const string BdiiBrowser::GLUE1 = "o=grid";
const string BdiiBrowser::GLUE2 = "o=glue";

const char* BdiiBrowser::ATTR_STATUS = "GlueSEStatus";
const char* BdiiBrowser::ATTR_SE = "GlueSEUniqueID";

// common for both GLUE1 and GLUE2
const char* BdiiBrowser::ATTR_OC = "objectClass";

const char* BdiiBrowser::ATTR_GLUE1_SERVICE = "GlueServiceUniqueID";
const char* BdiiBrowser::ATTR_GLUE1_SERVICE_URI = "GlueServiceURI";


const char* BdiiBrowser::ATTR_GLUE1_LINK = "GlueForeignKey";
const char* BdiiBrowser::ATTR_GLUE1_SITE = "GlueSiteUniqueID";
const char* BdiiBrowser::ATTR_GLUE1_HOSTINGORG = "GlueServiceHostingOrganization";

const char* BdiiBrowser::ATTR_GLUE2_SERVICE = "GLUE2ServiceID";
const char* BdiiBrowser::ATTR_GLUE2_SITE = "GLUE2ServiceAdminDomainForeignKey";


const char* BdiiBrowser::CLASS_SERVICE_GLUE2 = "GLUE2Service";
const char* BdiiBrowser::CLASS_SERVICE_GLUE1 = "GlueService";

const string BdiiBrowser::false_str = "false";

// "(| (%sAccessControlBaseRule=VO:%s) (%sAccessControlBaseRule=%s) (%sAccessControlRule=%s)"

const string BdiiBrowser::FIND_SE_STATUS(string se) {

	stringstream ss;
	ss << "(&(" << BdiiBrowser::ATTR_SE << "=*" << se << "*))";
	return ss.str();
}
const char* BdiiBrowser::FIND_SE_STATUS_ATTR[] = {BdiiBrowser::ATTR_STATUS, 0};


const string BdiiBrowser::FIND_SE_SITE_GLUE2(string se) {
	stringstream ss;
	ss << "(&";
	ss << "(" << BdiiBrowser::ATTR_OC << "=" << CLASS_SERVICE_GLUE2 << ")";
	ss << "(" << ATTR_GLUE2_SERVICE << "=*" << se << "*)";
	ss << ")";

	return ss.str();
}
const char* BdiiBrowser::FIND_SE_SITE_ATTR_GLUE2[] = {BdiiBrowser::ATTR_GLUE2_SITE, 0};

const string BdiiBrowser::FIND_SE_SITE_GLUE1(string se) {
	stringstream ss;
	ss << "(&";
	ss << "(" << BdiiBrowser::ATTR_OC << "=" << CLASS_SERVICE_GLUE1 << ")";
	ss << "(|(" << ATTR_GLUE1_SERVICE << "=*" << se << "*)";
	ss << "(" << ATTR_GLUE1_SERVICE_URI << "=*" << se << "*))";
	ss << ")";
	return ss.str();
}
const char* BdiiBrowser::FIND_SE_SITE_ATTR_GLUE1[] = {BdiiBrowser::ATTR_GLUE1_LINK, BdiiBrowser::ATTR_GLUE1_HOSTINGORG, 0};

BdiiBrowser::~BdiiBrowser() {

    disconnect();
}

bool BdiiBrowser::connect(string infosys, time_t sec) {

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
    if (ret != LDAP_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(ret) << " " << infosys << commit;
    	return false;
    }

    if (ldap_set_option(ld, LDAP_OPT_TIMEOUT, &search_timeout) != LDAP_OPT_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_TIMEOUT): " << ldap_err2string(ret) << " " << infosys << commit;
    }

    if (ldap_set_option(ld, LDAP_OPT_NETWORK_TIMEOUT, &timeout) != LDAP_OPT_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_NETWORK_TIMEOUT): " << ldap_err2string(ret) << " " << infosys << commit;
    }

    // set the keep alive if it has been set to 'true' in the fts3config file
    if (theServerConfig().get<bool>("BDIIKeepAlive")) {

    	int val = keepalive_idle;
		if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_IDLE,(void *) &val) != LDAP_OPT_SUCCESS) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_IDLE): " << ldap_err2string(ret) << " " << infosys << commit;
		}

		val = keepalive_probes;
		if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_PROBES,(void *) &val) != LDAP_OPT_SUCCESS) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_PROBES): " << ldap_err2string(ret) << " " << infosys << commit;
		}

		val = keepalive_interval;
		if (ldap_set_option(ld, LDAP_OPT_X_KEEPALIVE_INTERVAL, (void *) &val) != LDAP_OPT_SUCCESS) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP set option failed (LDAP_OPT_X_KEEPALIVE_INTERVAL): " << ldap_err2string(ret) << " " << infosys << commit;
		}
    }

    berval cred;
    cred.bv_val = 0;
    cred.bv_len = 0;

    ret = ldap_sasl_bind_s(ld, 0, LDAP_SASL_SIMPLE, &cred, 0, 0, 0);
    if (ret != LDAP_SUCCESS) {
    	FTS3_COMMON_LOGGER_NEWLOG (ERR) << "LDAP error: " << ldap_err2string(ret) << " " << infosys << commit;
    	return false;
    }
    
    connected = true;

    return true;
}

void BdiiBrowser::disconnect() {

    if (ld) {
        ldap_unbind_ext_s(ld, 0, 0);
        ld = 0;
    }

    connected = false;
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

bool BdiiBrowser::reconnect() {

	waitIfBrowsing();

	if (connected) disconnect();
	bool ret = connect(theServerConfig().get<string>("Infosys"));

	notifyBrowsers();

	return ret;
}

bool BdiiBrowser::isValid() {	

	// if we are not connected the connection is not valid
	if (!connected) return false;
	// if the bdii host have changed it is also not valid
	if (theServerConfig().get<string>("Infosys") != this->infosys) return false;

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

	// first check glue2
	list< map<string, list<string> > > rs = browse< list<string> >(GLUE2, FIND_SE_SITE_GLUE2(se), FIND_SE_SITE_ATTR_GLUE2);

	if (!rs.empty()) {
		if (!rs.front()[ATTR_GLUE2_SITE].empty()) {
			string str =  rs.front()[ATTR_GLUE2_SITE].front();
			return str;
		}
	}

	// then check glue1
	rs = browse< list<string> >(GLUE1, FIND_SE_SITE_GLUE1(se), FIND_SE_SITE_ATTR_GLUE1);

	if (rs.empty()) return string();

	list<string> values = rs.front()[ATTR_GLUE1_LINK];
	string site = parseForeingKey(values, ATTR_GLUE1_SITE);

	if (site.empty() && !rs.front()[ATTR_GLUE1_HOSTINGORG].empty()) {
		site = rs.front()[ATTR_GLUE1_HOSTINGORG].front();
	}

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
