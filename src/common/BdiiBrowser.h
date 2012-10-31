/*
 * BdiiBrowser.h
 *
 *  Created on: Oct 25, 2012
 *      Author: simonm
 */

#ifndef BDIIBROWSER_H_
#define BDIIBROWSER_H_

#include <ldap.h>

#include <string>
#include <list>
#include <map>

namespace fts3 {
namespace common {

using namespace std;

class BdiiBrowser {

public:
	BdiiBrowser(string url = "ldap://lcg-bdii.cern.ch:2170", long timeout = 60);
	virtual ~BdiiBrowser();

	bool getSeStatus(string se);
	string getSiteName (string se);

private:

	void connect();
	void disconnect();
	void reconnect();

	bool isValid();

	// if we want all available attributes we leave attr = 0
	template<typename R>
	list< map<string, R> > browse(string query, const char **attr = 0, long timeout = 60);

	template<typename R>
	list< map<string, R> > parseBdiiResponse(LDAPMessage *reply);

	template<typename R>
	map<string, R> parseBdiiSingleEntry(LDAPMessage *entry);

	template<typename R>
	R parseBdiiEntryAttribute(berval **value);

	string parseForeingKey(list<string> values, const char *attr);

	map<string, string> seToSite;

	LDAP *ld;
	const long timeout;
	const string base;
	const string url;

	static const char* ATTR_OC;
	static const char* ATTR_NAME;
	static const char* ATTR_URI;
	static const char* ATTR_STATUS;
	static const char* ATTR_SE;
	static const char* ATTR_LINK;
	static const char* ATTR_SITE;
	static const char* ATTR_HOSTINGORG;

	static const string FIND_SE_STATUS(string se);
	static const char* FIND_SE_STATUS_ATTR[];

	static const string FIND_SE_SITE(string se);
	static const char* FIND_SE_SITE_ATTR[];
};


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

} /* namespace ws */
} /* namespace fts3 */
#endif /* BDIIBROWSER_H_ */
