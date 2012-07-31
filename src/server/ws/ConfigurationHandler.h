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
 * ConfigurationHandler.h
 *
 *  Created on: Jun 26, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATIONHANDLER_H_
#define CONFIGURATIONHANDLER_H_

#include <string>
#include <vector>
#include <map>
#include <set>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include "common/error.h"
#include "common/logger.h"

#include "db/generic/SingleDbInstance.h"

namespace fts3 { namespace ws {

using namespace boost;
using namespace std;
using namespace fts3::common;
using namespace db;


/**
 * The ConfigurationHandler class is used for handling
 * Configuration WebServices' requests
 */
class ConfigurationHandler {
public:

	/**
	 * Enumeration that enumerates all
	 * important configuration parameters
	 * in the regular expression that
	 * describes the JSON SE configuration
	 */
	enum {
		/// the whole JSON configuration (if the configuration matches the regular expression it is stored under index=0)
		ALL = 0,
		/// SE or SE group name
		NAME,
		/// the entity type: SE or SE group
		TYPE,
		/// group members (list of SE names)
		MEMBERS = 4,
		/// protocol parameters (substructure)
		PROTOCOL_PARAMETERS = 6,
		/// share configuration (substructure)
		SHARE = 8,
		/// share type ('public', 'vo' or 'pair')
		SHARE_TYPE,
		/// the share id
		SHARE_ID = 11,
		/// inbound credits
		IN = 13,
		/// outbound credits
		OUT,
		/// share policy (exclusive or shared)
		POLICY
	};

	/**
	 * The enumeration corresponding to protocol parameters
	 */
	enum ProtocolParameters {
		BANDWIDTH = 0,
		NOSTREAMS,
		TCP_BUFFER_SIZE,
		NOMINAL_THROUGHPUT,
		BLOCKSIZE,
		HTTP_TO,
		URLCOPY_PUT_TO,
		URLCOPY_PUTDONE_TO,
		URLCOPY_GET_TO,
		URLCOPY_GET_DONETO,
		URLCOPY_TX_TO,
		URLCOPY_TXMARKS_TO,
		URLCOPY_FIRST_TXMARK_TO,
		TX_TO_PER_MB,
		NO_TX_ACTIVITY_TO,
		PREPARING_FILES_RATIO
	};

	/// SE type: 'se'
	static const string SE_TYPE;
	/// SE group type: 'group'
	static const string GROUP_TYPE;

	/// publicshare: 'public'
	static const string PUBLIC_SHARE;
	/// voshare: 'vo'
	static const string VO_SHARE;
	/// pairshare: 'pair'
	static const string PAIR_SHARE;

	/**
	 * Constructor
	 *
	 * initializes the regular expression objects and the 'parameterNameToId' map
	 */
	ConfigurationHandler();

	/**
	 * Destructor
	 */
	virtual ~ConfigurationHandler();

	/**
	 * Parses the configuration string (JSON format)
	 * 	Throws an Err_Custom if the given configuration is in wrong format.
	 *
	 * It has to be called before 'add'!
	 *
	 * 	@param configuration - string containing the configuration in JSON format
	 */
	void parse(string configuration);

	/**
	 * Retrieves the variable name from JSON sub-configuration
	 *
	 * @param cfg - the string containing the sub-configuration
	 *
	 * @return the name of the variable
	 */
	string getName(string cfg);

	/**
	 * Retrieves a numeric value from JSON sub-configuration
	 *
	 * @param cfg - the string containing the sub-configuration
	 *
	 * @return the numeric value of the variable
	 */
	int getNumber(string cfg);

	/**
	 * Retrieves a string value from JSON sub-configuration
	 *
	 * @param cfg - the string containing the sub-configuration
	 *
	 * @return the string value of the variable
	 */
	string getString(string cfg );

	/**
	 * Retrieves a vector value from JSON sub-configuration
	 *
	 * @param cfg - the string containing the sub-configuration
	 *
	 * @return the vector value of the variable
	 */
	vector<string> getVector(string cfg);

	/**
	 * Retrieves a vector containing protocol parameters from
	 * 	 JSON parameters structure, if a protocol parameter is
	 * 	 in the configuration it is placed in the vector using
	 * 	 the 'parameterNameToId' map, otherwise a 0 value is
	 * 	 placed in the vector
	 *
	 * @param cfg - the string containing the sub-configuration
	 *
	 * @return the vector with the parameters
	 */
	vector<int> getParamVector(string cfg);


	/*
	 * Checks if the SE exists, if not adds it to the DB
	 *
	 * @param name - SE name (possibly with wildmarks)
	 *
	 * @return a set with all SE names matching the SE name pattern
	 */
	set<string> addSeIfNotExist(string name);


	/**
	 * Adds a configuration to the DB.
	 * 	First the 'parse' method has to be called
	 *
	 * @see parse
	 */
	void add();

	/**
	 * If a SE configuration is being handled 'add' calls 'addSeConfiguration'
	 *
	 * @see add
	 */
	void addSeConfiguration();

	/**
	 * If a SE group configuration is being handled 'add' calls 'addGroupConfiguration'
	 *
	 * @see add
	 */
	void addGroupConfiguration();

	/**
	 * Both 'addSeConfiguration' and 'addGroupConfiguration' are using this utility
	 * in order to add the share configuration to the DB.
	 *
	 * @see addSeConfiguration
	 * @see addGroupConfiguration
	 */
	void addShareConfiguration(set<string> matchingNames);

	/**
	 * Gets the whole configuration regarding all SEs and all SE groups from the DB.
	 * 	The configuration can also be restricted to a given SE and/or VO.
	 *
	 * @param vo - VO name
	 * @param se - SE name
	 *
	 * @return vector containing single configuration entries in JSON format
	 */
	vector<string> get(string vo, string name);


	/**
	 * Deletes the configuration specified by the argument
	 * 	Only share and protocol specific configurations will be deleted.
	 *
	 * @param cfg - the configuration that should be deleted
	 */
	void del ();

	/**
	 * Gets protocol configuration for the SE or SE group,
	 * 	First the 'parse' method has to be called
	 *
	 * @return a shared_ptr pointing on a SeProtocolConfig object
	 *
	 * @see parse
	 */
	shared_ptr<SeProtocolConfig> getProtocolConfig(string name);

private:

	/// regular expression describing the whole configuration in JSON format
	static const string cfg_exp;
	/// regular expression used for retrieving the variable name from sub-configuration
	static const string get_name_exp;
	/// regular expression used for retrieving the variable numeric value from sub-configuration
	static const string get_num_exp;
	/// regular expression used for retrieving the variable string value from sub-configuration
	static const string get_str_exp;
	/// regular expression used for retrieving the variable vector value from sub-configuration
	static const string get_vec_exp;
	/// regular expression used for retrieving the values from parameter structure
	static const string get_par_exp;

	/// JSON null string = 'null'
	static const string null_str;

	/// regular expression object corresponding to 'cfg_exp'
	regex cfg_re;
	/// regular expression object corresponding to 'get_name_exp'
	regex get_name_re;
	/// regular expression object corresponding to 'get_num_exp'
	regex get_num_re;
	/// regular expression object corresponding to 'get_str_exp'
	regex get_str_re;
	/// regular expression object corresponding to 'get_vec_exp'
	regex get_vec_re;
	/// regular expression object corresponding to 'get_par_exp'
	regex get_par_re;

	/// Pointer to the 'GenericDbIfce' singleton
	GenericDbIfce* db;

	/// SE or SE group name
	string name;
	/// entity type: 'se' or 'group'
	string type;
	/// group members (list of SE names'
	vector<string> members;

	/// share type: 'public', 'vo' or 'pair'
	string share_type;
	/// share id
	string share_id;
	/// inbound credits
	int in;
	/// outbound credits
	int out;
	/// policy (exclusive or shared)
	string policy;

	/// true if group members have been defined, false otherwise
	bool cfgMembers;
	/// true if protocol parameters have been given, false otherwise
	bool cfgProtocolParams;
	/// true if share configuration has been defined, false otherwise
	bool cfgShare;

	/// maps the parameter names to respective index
	const map<string, ProtocolParameters> parameterNameToId;
	/// all parameters that have been set (0 value indicates that the parameter has not been set)
	vector<int> parameters;
	/// number of available protocol parameters
	static const int PARAMETERS_NMB = 16;
};

}
}

#endif /* TEST_H_ */
