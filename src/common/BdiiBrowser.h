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

#include "common/ThreadSafeInstanceHolder.h"

#include <ldap.h>

#include <sys/types.h>

#include <string>
#include <list>
#include <map>


#include <boost/thread.hpp>

namespace fts3 {
namespace common {

using namespace std;
using namespace boost;
using namespace common;

class BdiiBrowser: public ThreadSafeInstanceHolder<BdiiBrowser> {

	friend class ThreadSafeInstanceHolder<BdiiBrowser>;

public:

	virtual ~BdiiBrowser();

	bool connect(string infosys, time_t sec = 60);
	void disconnect();

	bool isVoAllowed(string se, string vo);
	bool getSeStatus(string se);
	string getSiteName (string se);

	static const string GLUE1;
	static const string GLUE2;

private:

	void reconnect();
	bool isValid();

	// if we want all available attributes we leave attr = 0
	template<typename R>
	list< map<string, R> > browse(string base, string query, const char **attr = 0);

	template<typename R>
	list< map<string, R> > parseBdiiResponse(LDAPMessage *reply);

	template<typename R>
	map<string, R> parseBdiiSingleEntry(LDAPMessage *entry);

	template<typename R>
	R parseBdiiEntryAttribute(berval **value);

	string parseForeingKey(list<string> values, const char *attr);

	map<string, string> seToSite;

	LDAP *ld;
	timeval timeout;
	timeval search_timeout;
	string infosys;
	string url;

	int querying;
	mutex qm;
	condition_variable qv;

	void waitIfBrowsing();
	void notifyBrowsers();
	void waitIfReconnecting();
	void notifyReconnector();

	static const char* ATTR_OC;
	static const char* ATTR_NAME;
	static const char* ATTR_URI;
	static const char* ATTR_STATUS;
	static const char* ATTR_SE;
	static const char* ATTR_LINK;
	static const char* ATTR_SITE;
	static const char* ATTR_HOSTINGORG;

	static const char* CLASS_SERVICE;

	static const string FIND_SE_STATUS(string se);
	static const char* FIND_SE_STATUS_ATTR[];

	static const string FIND_SE_SITE(string se);
	static const char* FIND_SE_SITE_ATTR[];

	static const string false_str;
	bool inuse;

	BdiiBrowser() : querying(0) {}
	BdiiBrowser(BdiiBrowser const&);
	BdiiBrowser& operator=(BdiiBrowser const&);
};


} /* namespace ws */
} /* namespace fts3 */
#endif /* BDIIBROWSER_H_ */
