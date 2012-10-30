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

	bool checkSeStatus(string se);

private:

	void connect();
	void disconnect();
	void reconnect();

	bool isValid();

	list< map<string, string> > browse(string query, long timeout = 60);


	LDAP *ld;
	const long timeout;
	const string base;
	const string url;
	const char **attr;

	static const string FIND_SE_STATUS(string se);
	static const char* ATTR_STATUS;
	static const char* FIND_SE_STATUS_ATTR[];
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* BDIIBROWSER_H_ */
