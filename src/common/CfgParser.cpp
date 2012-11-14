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
 * ConfigurationParser.cpp
 *
 *  Created on: Aug 10, 2012
 *      Author: Michał Simon
 */

#include "CfgParser.h"

#include <sstream>

#include <boost/assign.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace fts3 { namespace common {

using namespace boost::assign;

const map< string, set <string> > CfgParser::initGroupMembers() {

	set<string> root = list_of
			("name")
			("members")
			;
//	set<string> protocol = list_of ("parameters") ("pair");
//	set<string> share = list_of ("type") ("id") ("in") ("out") ("policy");

	return map_list_of
			(string(), root)
//			("protocol", protocol)
//			("share", share)
			;
}

const map< string, set <string> > CfgParser::initTransfer() {

	set<string> root = list_of
			("config_name")
			("source_se")
			("destination_se")
			("source_group")
			("destination_group")
			("source_any")
			("destination_any")
			("active_transfers")
			("vo")
			("protocol")
			("state")
			;

	return map_list_of
			(string(), root)
			;
}

const map< string, set <string> > CfgParser::initSeTransfer() {

	set<string> root = list_of
			("config_name")
			("group")
			("se")
			("vo")
			("active_transfers")
			;

	return map_list_of
			(string(), root)
			;
}

const map<string, set<string> > CfgParser::GROUP_MEMBERS_ALLOWED = CfgParser::initGroupMembers();
const map<string, set<string> > CfgParser::TRANSFER_ALLOWED = CfgParser::initTransfer();
const map<string, set<string> > CfgParser::SE_TRANSFER_ALLOWED = CfgParser::initSeTransfer();

CfgParser::CfgParser(string configuration) {

	// break into lines to give later better feedback to users
	replace_all(configuration, ",", ",\n");

	// store the lines in a vector
	vector<string> lines;
	char_separator<char> sep("\n");
	tokenizer< char_separator<char> > tokens(configuration, sep);
	tokenizer< char_separator<char> >::iterator it;

	for(it = tokens.begin(); it != tokens.end(); it++) {
		string s = *it;
		lines.push_back(s);
	}

	// put the configuration into a stream
	stringstream ss;
	ss << configuration;

	try {
		// parse
		read_json(ss, pt);

	} catch(json_parser_error& ex) {
		// handle errors in JSON format
		string msg =
				ex.message() +
				"(around: '" + lines[ex.line() - 1] + "')"
				;

		throw Err_Custom(msg);
	}

	if (validate(pt, TRANSFER_ALLOWED)) {
		type = TRANSFER_CFG;
		return;
	}

	if (validate(pt, GROUP_MEMBERS_ALLOWED)) {
		type = GROUP_MEMBERS_CFG;
		return;
	}

	if (validate(pt, SE_TRANSFER_ALLOWED)) {
		type = SE_TRANSFER_CFG;
		return;
	}

	type = NOT_A_CFG;
}

CfgParser::~CfgParser() {

}

optional< tuple <string, bool> > CfgParser::getSource() {

	tuple<string, bool> ret;

	optional<string> val = pt.get_optional<string>("source_se");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = false;
		return ret;
	}

	val = pt.get_optional<string>("source_group");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = true;
		return ret;
	}

	val = pt.get_optional<string>("source_any");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = false;
		return ret;
	}

	return optional< tuple<string, bool> >();
}

optional< tuple <string, bool> > CfgParser::getDestination() {

	tuple<string, bool> ret;

	optional<string> val = pt.get_optional<string>("destination_se");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = false;
		return ret;
	}

	val = pt.get_optional<string>("destination_group");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = true;
		return ret;
	}

	val = pt.get_optional<string>("destination_any");
	if (val) {
		boost::get<0>(ret) = *val;
		boost::get<1>(ret) = false;
		return ret;
	}

	return optional< tuple<string, bool> >();
}

bool CfgParser::validate(ptree pt, map< string, set <string> > allowed, string path) {

	// get the allowed names
	set<string> names;
	const map< string, set<string> >::const_iterator m_it = allowed.find(path);
	if (m_it != allowed.end()) {
		names = m_it->second;
	}

	ptree::iterator it;
	for (it = pt.begin(); it != pt.end(); it++) {
		pair<string, ptree> p = *it;

		// if it's an array entry just continue
		if (p.first.empty()) continue;

		// validate the name
		if (!names.count(p.first)) {
			string msg = "unexpected identifier: " + p.first;
			if (!path.empty()) msg += " in " + path + " object";
//			throw Err_Custom(msg);
			return false;
		}

		if (p.second.empty()) {
			// check if it should be an object or a value
			if(allowed.find(p.first) != allowed.end()) {
//				throw Err_Custom("A member object was expected in " + p.first);
				return false;
			}
		} else {
			// validate the child
			if (!validate(p.second, allowed, p.first)) {
				return false;
			}
		}
	}

	return true;
}

}
}
