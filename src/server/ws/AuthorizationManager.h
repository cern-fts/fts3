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
#include <utility>

#include <boost/tokenizer.hpp>

namespace fts3 { namespace ws {

using namespace fts3::common;
using namespace std;
using namespace boost;

class AuthorizationManager : public ThreadSafeInstanceHolder<AuthorizationManager> {

	friend class ThreadSafeInstanceHolder<AuthorizationManager>;

public:

	enum Level {
		NONE, // access has not been granted
		PRV,  // access granted only for private resources
		VO,   // access granted only for VO's resources
		ALL   // access granted
	};

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
	Level authorize(soap* soap, string operation);

	static const string ALL_SCOPE;
	static const string VO_SCOPE;
	static const string PRS_SCOPE;

	static const string PUBLIC_ACCESS;

	static const string DELEG_OP;
	static const string TRANSFER_OP;
	static const string CONFIG_OP;

	static const string WILD_CARD;

private:

	Level stringToScope(string s);

	template<typename R> R get(string e);

	Level check(string role, string operation);

	AuthorizationManager();
	AuthorizationManager(const AuthorizationManager&){};
	AuthorizationManager& operator=(const AuthorizationManager&){return *this;};

	const set<string> vos;
	const map<string, map<string, Level> > access;

	set<string> vostInit();
	// enum is by default initialized to 0, so even if a role is not in the 'access' map NON will be returned
	map<string, map<string, Level> > accessInit();

};


}
}

#endif /* AUTHORIZATIONMANAGER_H_ */
