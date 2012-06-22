/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#ifndef GSOAPDELEGATIONHANDLER_H_
#define GSOAPDELEGATIONHANDLER_H_

#include <string>
#include <gridsite.h>

#include "ws/gsoap_stubs.h"

namespace fts3 { namespace ws {

using namespace std;

class GSoapDelegationHandler {
public:
	GSoapDelegationHandler(soap* ctx);
	virtual ~GSoapDelegationHandler();

	string makeDelegationId();
	string handleDelegationId(string delegationId);
	bool checkDelegationId(string delegationId);

	string x509ToString(X509* cert);
	string addKeyToProxyCertificate(string proxy, string key);
	time_t readTerminationTime(string proxy);
	string fqansToString(vector<string> attrs);

	string getProxyReq(string delegationId);
	string renewProxyReq(string delegationId);
	time_t getTerminationTime(string delegationId);
	delegation__NewProxyReq* getNewProxyReq();
	void putProxy(string delegationId, string proxy);
	void destroy(string delegationId);

private:
	soap* ctx;
	string dn;
	vector<string> attrs;
};

}
}

#endif /* GSOAPDELEGATIONHANDLER_H_ */
