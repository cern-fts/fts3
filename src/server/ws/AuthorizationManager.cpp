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
 * AuthorizationManager.cpp
 *
 *  Created on: Jul 18, 2012
 *      Author: simonm
 */

#include "AuthorizationManager.h"
#include "GSoapDelegationHandler.h"

#include "common/error.h"
#include "common/logger.h"

#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

using namespace fts3::ws;
using namespace boost;
using namespace boost::assign;

AuthorizationManager::AuthorizationManager() : voNames(list_of
		("dteam")
		("cms")
		("atlas")
		("lhcb").to_container(voNames)
	) {
	// initialization list does everything :)
}

AuthorizationManager::~AuthorizationManager() {

}

void AuthorizationManager::authorize(soap* soap) {

	GSoapDelegationHandler handler (soap);
	string vo = handler.getClientVo();
	to_lower(vo);

	if (!voNames.count(vo)) {
		throw Err_Custom("Authorization failed, access was not granted.");
	}
}

