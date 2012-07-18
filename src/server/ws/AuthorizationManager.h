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

#include "common/InstanceHolder.h"
#include "gsoap_stubs.h"

#include <set>

namespace fts3 { namespace ws {

using namespace fts3::common;
using namespace std;

class AuthorizationManager : public InstanceHolder<AuthorizationManager> {

	friend class InstanceHolder<AuthorizationManager>;

public:
	virtual ~AuthorizationManager();
	void authorize(soap* soap);

private:
	AuthorizationManager();
	AuthorizationManager(const AuthorizationManager&){};
	AuthorizationManager& operator=(const AuthorizationManager&){return *this;};

	const set<string> voNames;
};

}
}

#endif /* AUTHORIZATIONMANAGER_H_ */
