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

const map<string, set <string> > CfgParser::standaloneSeCfgTokens = CfgParser::initStandaloneSeCfgTokens();
const map<string, set <string> > CfgParser::standaloneGrCfgTokens = CfgParser::initStandaloneGrCfgTokens();
const map<string, set <string> > CfgParser::sePairCfgTokens = CfgParser::initSePairCfgTokens();
const map<string, set <string> > CfgParser::grPairCfgTokens = CfgParser::initGrPairCfgTokens();

const set<string> CfgParser::allTokens =
		list_of
		("se")
		("group")
		("members")
		("active")
		("in")
		("out")
		("share")
		("protocol")
		("symbolic_name")
		("source_se")
		("destination_se")
		("source_group")
		("destination_group")
		;

const map< string, set <string> > CfgParser::initStandaloneSeCfgTokens() {

	set<string> root = list_of
			("se")
			("active")
			("in")
			("out")
			;

	set<string> cfg = list_of
			("share")
			("protocol")
			;

	return map_list_of
			(string(), root)
			("in", cfg)
			("out", cfg)
			;
}

const map< string, set <string> > CfgParser::initStandaloneGrCfgTokens() {

	set<string> root = list_of
			("group")
			("members")
			("active")
			("in")
			("out")
			;

	set<string> cfg = list_of
			("share")
			("protocol")
			;

	return map_list_of
			(string(), root)
			("in", cfg)
			("out", cfg)
			;
}

const map< string, set <string> > CfgParser::initSePairCfgTokens() {

	set<string> root = list_of
			("symbolic_name")
			("active")
			("source_se")
			("destination_se")
			("share")
			("protocol")
			;

	return map_list_of
			(string(), root)
			;
}

const map< string, set <string> > CfgParser::initGrPairCfgTokens() {

	set<string> root = list_of
			("symbolic_name")
			("active")
			("source_group")
			("destination_group")
			("share")
			("protocol")
			;

	return map_list_of
			(string(), root)
			;
}

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

	if (validate(pt, standaloneSeCfgTokens)) {
		type = STANDALONE_SE_CFG;
		return;
	}

	if (validate(pt, standaloneGrCfgTokens)) {
		type = STANDALONE_GR_CFG;
		return;
	}

	if (validate(pt, sePairCfgTokens)) {
		type = SE_PAIR_CFG;
		return;
	}

	if (validate(pt, grPairCfgTokens)) {
		type = GR_PAIR_CFG;
		return;
	}

	type = NOT_A_CFG;
}

CfgParser::~CfgParser() {

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
			if (!allTokens.count(p.first)) {
				string msg = "unexpected identifier: " + p.first;
				if (!path.empty()) msg += " in " + path + " object";
				throw Err_Custom(msg);
			}
			return false;
		}

		if (p.second.empty()) {
			// check if it should be an object or a value
			if(allowed.find(p.first) != allowed.end()) {
				throw Err_Custom("A member object was expected in " + p.first);
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
