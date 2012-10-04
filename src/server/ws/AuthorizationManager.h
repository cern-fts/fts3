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
 *
 * AuthorizationManager.h
 *
 *  Created on: Jul 18, 2012
 *      Author: simonm
 */

#ifndef AUTHORIZATIONMANAGER_H_
#define AUTHORIZATIONMANAGER_H_

#include "common/ThreadSafeInstanceHolder.h"
#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <set>
#include <vector>

namespace fts3 { namespace ws {

using namespace fts3::common;
using namespace std;

class AuthorizationManager : public ThreadSafeInstanceHolder<AuthorizationManager> {

	friend class ThreadSafeInstanceHolder<AuthorizationManager>;

public:
	virtual ~AuthorizationManager();

	/**
	 * Authorize the operation requested by the user.
	 *
	 * A root user of the server hosting the fts3 service if automatically
	 * authorized, an exception is the transfer-submit operation!
	 *
	 * @param soap - the soap context
	 * @param submit - should be true if the submit operation is beeing authorized
	 *
	 */
	void authorize(soap* soap, bool submit = false);

private:
	AuthorizationManager();
	AuthorizationManager(const AuthorizationManager&){};
	AuthorizationManager& operator=(const AuthorizationManager&){return *this;};

	const vector<string> voNameList;
	const set<string> voNameSet;
};

}
}

#endif /* AUTHORIZATIONMANAGER_H_ */
