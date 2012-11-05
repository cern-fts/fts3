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
 *      Author: Michal Simon
 */

#include "AuthorizationManager.h"
#include "CGsiAdapter.h"

#include "common/error.h"
#include "common/logger.h"
#include "config/serverconfig.h"

#include <boost/algorithm/string.hpp>

#include "config/serverconfig.h"

using namespace config;

namespace fts3 { namespace ws {

const string AuthorizationManager::ALL_SCOPE = "all";
const string AuthorizationManager::VO_SCOPE = "vo";
const string AuthorizationManager::PRS_SCOPE;

const string AuthorizationManager::PUBLIC_ACCESS = "roles.Public";

const string AuthorizationManager::DELEG_OP = "deleg";
const string AuthorizationManager::TRANSFER_OP = "transfer";
const string AuthorizationManager::CONFIG_OP = "config";

const string AuthorizationManager::WILD_CARD = "*";

template<>
vector<string> AuthorizationManager::get< vector<string> >(string cfg) {

	char_separator<char> sep(";");
	tokenizer< char_separator<char> > tokens(cfg, sep);
	tokenizer< char_separator<char> >::iterator it;

	std::vector<string> ret;
	for (it = tokens.begin(); it != tokens.end(); it++) {
		ret.push_back(*it);
	}

	return ret;
}

template<>
string AuthorizationManager::get<string>(string cfg) {
	size_t pos = cfg.find(':');
	if (pos != string::npos) {
		return cfg.substr(pos + 1);
	} else {
		return cfg;
	}
}

template<>
AuthorizationManager::Level AuthorizationManager::get<AuthorizationManager::Level>(string cfg) {
	size_t pos = cfg.find(':');
	if (pos != string::npos) {
		return stringToScope(cfg.substr(0, pos));
	} else {
		return PRV;
	}
}

set<string> AuthorizationManager::vostInit() {

	// parse the authorized vo list
	string volist = theServerConfig().get<string>("AuthorizedVO");
	vector<string> voNameList = get< vector<string> >(volist);

	return set <string> (voNameList.begin(), voNameList.end());
}

map<string, map<string, AuthorizationManager::Level> > AuthorizationManager::accessInit() {

	map<string, map<string, Level> > ret;

	// roles.* is a regular expression for all role entries
	map<string, string> rolerights = theServerConfig().get< map<string, string> > ("roles.*");
	if (!rolerights.empty()) {
		map<string, string>::iterator it;
		for (it = rolerights.begin(); it != rolerights.end(); it++) {

			map<string, Level> rights;

			vector<string> r = get< vector<string> >(it->second);
			vector<string>::iterator r_it;
			for (r_it = r.begin(); r_it != r.end(); r_it++) {
				rights[get<string>(*r_it)] = get<Level>(*r_it);
			}

			ret[it->first] = rights;
		}
	}

	return ret;
}

AuthorizationManager::AuthorizationManager() : vos(vostInit()), access(accessInit()) {

}

AuthorizationManager::~AuthorizationManager() {

}

AuthorizationManager::Level AuthorizationManager::stringToScope(string s) {
	if (s == ALL_SCOPE) return ALL;
	if (s == VO_SCOPE) return VO;
	return PRV;
}

AuthorizationManager::Level AuthorizationManager::check(string role, string operation) {

	map< string, map<string, Level> >::const_iterator a_it;

	// check if the role is specified in fts3config file
	a_it = access.find(role);
	if (a_it == access.end()) return NONE;

	map<string, Level>::const_iterator l_it;

	Level ret = NONE;

	// check is there is a wild card
	l_it = a_it->second.find(WILD_CARD);
	if (l_it != a_it->second.end()) {
		ret = l_it->second;
	}

	// check if the operation is given directly
	l_it = a_it->second.find(operation);
	if (l_it != a_it->second.end()) {
		if (l_it->second > ret) ret = l_it->second;
	}

	// return the higher access level
	return ret;
}

AuthorizationManager::Level AuthorizationManager::authorize(soap* soap, string operation) {

	CGsiAdapter cgsi(soap);

	// root is authorized to do anything but delegations
	if(cgsi.isRoot()) {
		if (operation != DELEG_OP) return ALL;
		string msg = "Authorization failed, a host certificate has been used to submit a transfer!";
		throw Err_Custom(msg);
	}

	// if the VO authorization list was not specified or a wildcard was used ...
	if (!vos.empty() && !vos.count("*")) {

		string vo = cgsi.getClientVo();
		to_lower(vo);

		if (!vos.count(vo)) {

			string msg = "Authorization failed, access was not granted. ";
			msg += "(Please check if the fts3 configuration file contains the VO: '";
			msg += vo;
			msg += "' and if the right delimiter was used!)";

			throw Err_Custom(msg);
		}
	}

	// check if the access is public
	Level lvl = check(PUBLIC_ACCESS, operation);

	// check if the user has a role that is granting him the access
	vector<string> roles = cgsi.getClientRoles();
	if (!roles.empty()) {
		vector<string>::iterator it;
		for (it = roles.begin(); it != roles.end(); it++) {
			Level tmp = check(*it, operation);
			if (tmp > lvl) lvl = tmp;
		}
	}

	return lvl;

	// if access was not granted throw an exception
	if (lvl != NONE) return lvl;
	else {
		string msg = "Authorization failed, access was not granted. ";
		msg += "(The user has not rights to perform ";
		msg += operation;
		msg += 	" operation)";
		throw Err_Custom(msg);
	}
}

}
}



