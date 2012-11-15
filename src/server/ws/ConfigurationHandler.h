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
 *      Author: Michał Simon
 */

#ifndef CONFIGURATIONHANDLER_H_
#define CONFIGURATIONHANDLER_H_

#include <string>
#include <vector>
#include <map>
#include <set>

#include <boost/assign.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tuple/tuple.hpp>

#include "common/error.h"
#include "common/logger.h"
#include "common/CfgBlocks.h"
#include "common/CfgParser.h"

#include "db/generic/SingleDbInstance.h"

namespace fts3 { namespace ws {

using namespace boost;
using namespace boost::assign;
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
	 * Functional object that replaces fts3 wildcard '*' with sql wild card '%'
	 */
	struct WildcardReplacer {

		/**
		 * Replaces fts wildcard ('*') with sql wildcard ('%') in a given string
		 *
		 * @param str - the string that is being processed
		 */
		void operator() (string& str) {
			replace_all(str, CfgBlocks::FTS_WILDCARD, CfgBlocks::SQL_WILDCARD);
		}
	} replacer; //< WildcardReplacer object

	/**
	 * protocol parameters
	 */
	struct ProtocolParameters {
		static const string BANDWIDTH;
		static const string NOSTREAMS;
		static const string TCP_BUFFER_SIZE;
		static const string NOMINAL_THROUGHPUT;
		static const string BLOCKSIZE;
		static const string HTTP_TO;
		static const string URLCOPY_PUT_TO;
		static const string URLCOPY_PUTDONE_TO;
		static const string URLCOPY_GET_TO;
		static const string URLCOPY_GET_DONETO;
		static const string URLCOPY_TX_TO;
		static const string URLCOPY_TXMARKS_TO;
		static const string URLCOPY_FIRST_TXMARK_TO;
		static const string TX_TO_PER_MB;
		static const string NO_TX_ACTIVITY_TO;
		static const string PREPARING_FILES_RATIO;
	};

	/**
	 * Constructor
	 *
	 * initializes the regular expression objects and the 'parameterNameToId' map
	 */
	ConfigurationHandler(string dn);

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
	 * Adds a configuration to the DB.
	 * 	First the 'parse' method has to be called
	 *
	 * @see parse
	 */
	void add();

	/**
	 * If transfer configuration has been submitted this method is used to add the configuration to DB
	 */
	void addTransferConfiguration();

	/**
	 * Adds a sub-configuration for a pair configuration
	 */
	void addSeTransferConfiguration();

	/**
	 * Gets the whole configuration regarding all SEs and all SE groups from the DB.
	 * 	The configuration can also be restricted to a given SE and/or VO.
	 *
	 * @param vo - VO name
	 * @param se - SE name
	 *
	 * @return vector containing single configuration entries in JSON format
	 */
	vector<string> get(string cfg_name, string vo);

	vector<string> get(string src, string dest, string vo);

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
	shared_ptr<SeProtocolConfig> getProtocolConfig();

	/**
	 * Gets the SE / SE group name
	 *
	 * @return se name
	 */
	string getName() {
		return "dummy"; // TODO it's used for authorization! should be the se name (?)
	}

private:

	string buildPairCfg(SeConfig* cfg, SeProtocolConfig* prot);

	string buildGroupCfg(string name, vector<string> members);

	string buildSeCfg(SeGroup* sg);

	vector<string> getGroupCfg(string cfg_name, string name, string vo);

	vector<string> doGet(SeConfig* cfg);

	/**
	 * Converts boolean to string:
	 * 	true 	-> 'true'
	 * 	false 	-> 'false'
	 *
	 * 	@param b - boolean value
	 *
	 * 	@return if the parameter was true - 'true', otherwise 'false'
	 */
	string str(bool b);

	/**
	 * Checks if the SE exists if not adds it to the DB
	 */
	void checkSe(string name);

	/**
	 *
	 */
	void checkGroup(string name);

	/**
	 *
	 */
	void checkEntity(tuple<string, bool> ent);


	/// Pointer to the 'GenericDbIfce' singleton
	GenericDbIfce* db;

	/// the whole cfg comand
	string all;
	/// configuration name
	optional<string> cfg_name;
	/// SE or SE group name
	optional<string> name;
	/// group members (list of SE names'
	optional< vector<string> > members;
	/// member name
	optional<string> member;

	/// the source of the transfer config
	optional< tuple<string, bool> > source;
	/// the destination of the transfer config
	optional< tuple<string, bool> > destination;
	/// number of active transfers
	optional<int> active_transfers;
	/// vo
	optional<string> vo;
	/// protocol parameters
	optional< map<string, int> > protocol;
	/// configuration state
	optional<string> state;

	/// type of the configuration that is being submitted
	CfgParser::CfgType type;

	/// number of available protocol parameters
	static const int PARAMETERS_NMB = 16;

	/// user DN
	string dn;

	/// number of SQL updates triggered by configuration command
	int updateCount;
	/// number of SQL inserts triggered by configuration command
	int insertCount;
	/// number of SQL deletes triggered by configuration command
	int deleteCount;
	/// number of debug cmd triggered by configuration command
	int debugCount;
};

}
}

#endif /* TEST_H_ */
