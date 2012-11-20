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
 * ConfigurationParser.h
 *
 *  Created on: Aug 10, 2012
 *      Author: Michał Simon
 */

#ifndef CONFIGURATIONPARSER_H_
#define CONFIGURATIONPARSER_H_

#include "common/error.h"

#include <set>
#include <map>
#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>

namespace fts3 { namespace common {

using namespace std;
using namespace boost;
using namespace boost::property_tree;


/**
 * CfgParser is a wrapper class for boost ptree.
 *
 * It allows to parse a configuration string in JSON format.
 * After parsing a given value can be accessed using a specific path
 * e.g.: 'share.in' will provide access to the value inbound
 * value specified in share JSON object
 * (see https://svnweb.cern.ch/trac/fts3/wiki/UserGuide for the JSON schema)
 *
 * Access to JSON array is supported.
 *
 */
class CfgParser {

public:

	enum CfgType {
		NOT_A_CFG,
		STANDALONE_SE_CFG,
		STANDALONE_GR_CFG,
		SE_PAIR_CFG,
		GR_PAIR_CFG
	};

	/**
	 * Constructor.
	 * Parses the given JSON configuration.
	 *
	 * @param configuration - the configuration in JSON format
	 */
	CfgParser(string configuration);

	/**
	 * Destructor.
	 */
	virtual ~CfgParser();

	/**
	 * Gets the specific value from the JSON object
	 *
	 * @param path - path that specifies the value, e.g. 'share.in'
	 * @param T - the expected type of the value (int, string, bool, vector and map are supported)
	 *
	 * @return the value
	 */
	template <typename T>
	T get(string path);

	/**
	 * Gets the specific value for an optional object in JSON configuration
	 *
	 * @param path - path that specifies the value, e.g. 'share.in'
	 *
	 * @return an instance of optional which holds the value
	 */
	optional<string> get_opt(string path);

	/**
	 *
	 */
	CfgType getCfgType() {
		return type;
	}

private:

	CfgType type;

	/**
	 * Validates the ptree object. Checks if the configuration format is OK.
	 *
	 * @param pt - the ptree that has to be validated
	 * @param allowed - a collection of fileds name in the cfg JASON
	 * 					characteristic for a given type of configuration
	 * @param path - the path in main ptree (by default root)
	 *
	 * @return true if it's a configuration of a given type, false otherwise
	 */
	bool validate(ptree pt, map< string, set <string> > allowed, string path = string());

	/// The object that contains the parsed configuration
	ptree pt;

	/// the tokens used in standalone SE configuration
	static const map<string, set <string> > standaloneSeCfgTokens;
	/// the tokens used in standalone SE groupconfiguration
	static const map<string, set <string> > standaloneGrCfgTokens;
	/// the tokens used in se pair configuration
	static const map<string, set <string> > sePairCfgTokens;
	/// the tokens used in se group pair configuration
	static const map<string, set <string> > grPairCfgTokens;
	/// all the allowed tokens
	static const set<string> allTokens;

	/// initializes allowed JSON members for se config
	static const map< string, set <string> > initStandaloneSeCfgTokens();
	/// initializes allowed JSON members for se group config
	static const map< string, set <string> > initStandaloneGrCfgTokens();
	/// initializes allowed JSON members for se pair
	static const map< string, set <string> > initSePairCfgTokens();
	/// initializes allowed JSON members for se group pair
	static const map< string, set <string> > initGrPairCfgTokens();
};

template <typename T>
T CfgParser::get(string path) {

	T v;
	try {

		v = pt.get<T>(path);

	} catch (ptree_bad_path& ex) {
		throw Err_Custom("The " + path + " has to be specified!");

	} catch (ptree_bad_data& ex) {
		// if the type of the value is wrong throw an exception
		throw Err_Custom("Wrong value type of " + path);
	}

	return v;
}

template <>
inline vector<string> CfgParser::get< vector<string> >(string path) {

	vector<string> ret;

	optional<ptree&> value = pt.get_child_optional(path);
	if (!value.is_initialized()) {
		// the vector member was not specified in the configuration
		throw Err_Custom("The " + path + " has to be specified!");
	}
	ptree& array = value.get();

	// check if the node has a value,
	// accordingly to boost it should be empty if array syntax was used in JSON
	string wrong = array.get_value<string>();
	if (!wrong.empty()) {
		throw Err_Custom("Wrong value: '" + wrong + "'");
	}

	ptree::iterator it;
	for (it = array.begin(); it != array.end(); it++) {
		pair<string, ptree> v = *it;

		// check if the node has a name,
		// accordingly to boost it should be empty if object weren't
		// members of the array (our case)
		if (!v.first.empty()) {
			throw Err_Custom("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
		}

		// check if the node has children, it should only have a value!
		if (!v.second.empty()) {
			throw Err_Custom("Unexpected object in array '" + path + "' (only a list of values was expected)");
		}

		ret.push_back(v.second.get_value<string>());
	}

	return ret;
}

template <>
inline map <string, int> CfgParser::get< map<string, int> >(string path) {

	map<string, int> ret;

	optional<ptree&> value = pt.get_child_optional(path);
	if (!value.is_initialized()) throw Err_Custom("The " + path + " has to be specified!");
	ptree& array = value.get();

	// check if the node has a value,
	// accordingly to boost it should be empty if array syntax was used in JSON
	string wrong = array.get_value<string>();
	if (!wrong.empty()) {
		throw string ("Wrong value: '" + wrong + "'");
	}

	ptree::iterator it;
	for (it = array.begin(); it != array.end(); it++) {
		pair<string, ptree> v = *it;

		// check if the node has a name,
		// accordingly to boost it should be empty if object weren't
		// members of the array (our case)
		if (!v.first.empty()) {
			throw Err_Custom("An array was expected, instead an object was found (at '" + path + "', name: '" + v.first + "')");
		}

		// check if there is a value,
		// the value should be empty because only a 'key:value' object should be specified
		if (!v.second.get_value<string>().empty()) {
			throw Err_Custom("'{key:value}' object was expected, not just the value");
		}

		// there should be only one child the 'key:value' object
		if (v.second.size() != 1) {
			throw Err_Custom("In array '" + path + "' only ('{key:value}' objects were expected)");
		}

		pair<string, ptree> kv = v.second.front();
		try {
			ret[kv.first] = kv.second.get_value<int>();
		} catch(ptree_bad_data& ex) {
			throw Err_Custom("Wrong value type of " + kv.first);
		}
	}

	return ret;
}

}
}

#endif /* CONFIGURATIONPARSER_H_ */
