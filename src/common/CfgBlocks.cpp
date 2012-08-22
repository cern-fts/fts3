/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
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
 * CfgBuilder.cpp
 *
 *  Created on: Aug 20, 2012
 *      Author: Michał Simon
 */

#include "CfgBlocks.h"

#include <boost/lexical_cast.hpp>

using namespace fts3::common;

const string CfgBlocks::FTS_WILDCARD = "*";
const string CfgBlocks::SQL_WILDCARD = "%";

const string CfgBlocks::SHARED_POLICY = "shared";
const string CfgBlocks::EXCLUSIVE_POLICY = "exclusive";

const string CfgBlocks::SE_TYPE = "se";
const string CfgBlocks::GROUP_TYPE = "group";

const string CfgBlocks::PUBLIC_SHARE_TYPE = "public";
const string CfgBlocks::VO_SHARE_TYPE = "vo";
const string CfgBlocks::PAIR_SHARE_TYPE = "pair";

const regex CfgBlocks::valueRegex ("\"in\":(\\d+),\"out\":(\\d+),\"policy\":\"(shared|exclusive)\"");
const regex CfgBlocks::idRegex ("\"type\":\"(public|vo|pair)\"(,\"id\":\"(.+)\")?");

const regex CfgBlocks::typeRegex("se|group");
const regex CfgBlocks::shareTypeRegex("public|vo|pair");
const regex CfgBlocks::policyRegex("shared|exclusive");

const regex CfgBlocks::fileUrlRegex(".+://([a-zA-Z0-9\\.-]+)(:\\d+)?/.+");

optional< tuple<int, int, string> > CfgBlocks::getShareValue(string val) {

	smatch what;
	if (regex_match(val, what, valueRegex, match_extra)) {

		// indexes are shifted by 1 because at index 0 is the whole string
		tuple<int, int, string> ret (
				lexical_cast<int>(what[INBOUND + 1]),
				lexical_cast<int>(what[OUTBOUND + 1]),
				what[POLICY + 1]
			);

		return ret;

	} else
		return optional< tuple<int, int, string> >();
}

optional< tuple<string, string> > CfgBlocks::getShareId(string id) {

	smatch what;
	if (regex_match(id, what, idRegex, match_extra)) {

		// indexes are shifted by 1 because at index 0 is the whole string
		tuple<string, string> ret (
				what[TYPE + 1],
				what[ID + 1]
			);

		return ret;

	} else
		return optional< tuple<string, string> >();

}

optional<string> CfgBlocks::fileUrlToSeName(string url) {

	smatch what;
	if (regex_match(url, what, fileUrlRegex, match_extra)) {

		// indexes are shifted by 1 because at index 0 is the whole string
		return string(what[1]);

	} else
		return string();
}

bool CfgBlocks::isValidType(string type) {
	smatch what;
	return regex_match(type, what, typeRegex, match_extra);
}

bool CfgBlocks::isValidShareType(string type) {
	smatch what;
	return regex_match(type, what, shareTypeRegex, match_extra);
}

bool CfgBlocks::isValidPolicy(string policy) {
	smatch what;
	return regex_match(policy, what, policyRegex, match_extra);
}

