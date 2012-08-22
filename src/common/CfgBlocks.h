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
 * CfgBuilder.h
 *
 *  Created on: Aug 20, 2012
 *      Author: Micha\u0142 Simon
 */

#ifndef CFGBUILDER_H_
#define CFGBUILDER_H_

#include <string>
#include <boost/regex.hpp>
#include "boost/tuple/tuple.hpp"
#include <boost/optional.hpp>


namespace fts3 { namespace common {

using namespace std;
using namespace boost;


/**
 *
 */
class CfgBlocks {
public:

	/**
	 * Share id enumeration
	 */
	enum Id {
		TYPE,
		ID
	};

	/**
	 * Share value enumeration
	 */
	enum Value {
		INBOUND,
		OUTBOUND,
		POLICY
	};

	/**
	 * Destructor
	 */
	virtual ~CfgBlocks(){};

	/**
	 * Constructs the share ID of any type
	 *
	 * @param type - the type of share
	 * @param id - the id of share (if empty the id part is being omitted)
	 *
	 * @return the share ID of given type and with given id
	 */
	static string shareId(string type, string id) {

		string ret = "\"type\":\"" + type + "\"";
		if (!id.empty()) {
			ret += ",\"id\":\"" + id + "\"";
		}

		return ret;
	}

	/**
	 * Constructs the share ID of voshare type
	 *
	 * @return the voshare ID string for given vo name
	 */
	static string voShare(string vo) {
		return shareId(VO_SHARE_TYPE, vo);
	};

	/**
	 * Constructs the share ID of publicshare type
	 *
	 * @return the publicshare ID string
	 */
	static string publicShare() {
		return shareId(PUBLIC_SHARE_TYPE, string());
	};

	/**
	 * Constructs the share ID of pairshare type
	 *
	 * @return  the pairshare ID string
	 */
	static string pairShare(string pair) {
		return shareId(PAIR_SHARE_TYPE, pair);
	}

	/**
	 * Gets the string with JSON format share value
	 *
	 * @param in - inbound
	 * @param out - outbound
	 * @param policy - share policy
	 *
	 * @return share value in JSON format
	 *
	 * @see FTS_WILDCARD, SQL_WILDCARD
	 */
	static string shareValue(string in, string out, string policy) {
		return "\"in\":" + in + ",\"out\":" + out + ",\"policy\":\"" + policy + "\"";
	}

	/**
	 * Retrieves the share value from JSON value configuration
	 * 	if the input string does not match JSON value format the
	 * 	returned value is not initialized
	 *
	 * 	@param val - JSON value configuration
	 *
	 * 	@return an optional tuple object containing the values (inbound, outbound,
	 * 			policy from JSON value configuration (if the string format was not
	 * 			correct the object is not initialized), in order to access the
	 * 			values use 'Value' enumeration e.g. get<INBOUND>(val.get())
	 *
	 * 	@see CfgBlocks::Value
	 */
	static optional< tuple<int, int, string> > getShareValue(string val);

	/**
	 * Retrieves the share id from JSON value configuration
	 * 	if the input string does not match JSON value format the
	 * 	returned value is not initialized
	 *
	 * 	@param id - JSON share id configuration
	 *
	 * 	@return an optional tuple object containing the id (type, id) from JSON
	 * 			share id configuration (if the string format was not correct the object
	 * 			is not initialized), in order to access the values use 'Id'
	 * 			enumeration e.g. get<TYPE>(id.get())
	 *
	 * 	@see CfgBlocks::Id
	 */
	static optional< tuple<string, string> > getShareId(string id);

	/**
	 * Checks if a given string is a valid configuration type
	 *
	 * @param type - configuration type
	 *
	 * @return true if it is a configuration type, otherwise false
	 */
	static bool isValidType(string type);

	/**
	 * Checks if a given string is a valid share type
	 *
	 * @param type - share type
	 *
	 * @return true if it is a share type, otherwise false
	 */
	static bool isValidShareType(string type);

	/**
	 * Checks if a given string is a valid policy
	 *
	 * @param policy - share policy
	 *
	 * @return true if it is a policy, otherwise false
	 */
	static bool isValidPolicy(string policy);


	static optional<string> fileUrlToSeName(string url);

	/// FTS wildcard
	static const string FTS_WILDCARD;

	/// SQL wildmark
	static const string SQL_WILDCARD;


	/// shared policy string
	static const string SHARED_POLICY;

	/// exclusive policy string
	static const string EXCLUSIVE_POLICY;


	/// configuration type 'se'
	static const string SE_TYPE;

	/// configuration type 'group'
	static const string GROUP_TYPE;


	/// share type 'public'
	static const string PUBLIC_SHARE_TYPE;

	/// share type 'vo'
	static const string VO_SHARE_TYPE;

	/// share type 'pair'
	static const string PAIR_SHARE_TYPE;

private:

	/// the regex object describing share vale
	static const regex valueRegex;

	/// the regex object describing share id
	static const regex idRegex;

	/// the regex object describing configuration type
	static const regex typeRegex;

	/// the regex object describing share type
	static const regex shareTypeRegex;

	/// the regex object describing share policy
	static const regex policyRegex;


	/// the regex object describing file url (TODO should not be in this class, think were to put it)
	static const regex fileUrlRegex;

	/**
	 * Constructor
	 */
	CfgBlocks(){};
};

}
}

#endif /* CFGBUILDER_H_ */
