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
 * ConfigurationHandler.cpp
 *
 *  Created on: Jun 26, 2012
 *      Author: Michał Simon
 */

#include "ConfigurationHandler.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

using namespace fts3::ws;

const string ConfigurationHandler::cfg_exp =
		"\\s*\\{\\s*"
			"(\"name\"\\s*:\\s*\"[\\*a-zA-Z0-9\\.-]+\")\\s*,\\s*"
			"(\"type\"\\s*:\\s*\"(se|group)\")\\s*,?\\s*"
			"(\"active\"\\s*:\\s*\"(true|false)\")?\\s*,?\\s*"
			"(\"members\"\\s*:\\s*\\[(\\s*\"[\\*a-zA-Z0-9\\.-]+\"\\s*,?\\s*)+\\])?\\s*,?\\s*"
			"("
				"\"protocol\"\\s*:\\s*"
				"\\s*\\{\\s*"
					"((\"pair\"\\s*:\\s*\"[\\*a-zA-Z0-9\\.-]+\")\\s*,\\s*)?"
					"("
						"\"parameters\"\\s*:\\s*\\["
							"("
							"\\s*\\{\\s*"
								"\"name\"\\s*:\\s*\"[a-zA-Z0-9\\.-_]+\"\\s*,\\s*"
								"\"value\"\\s*:\\s*\\d+\\s*"
							"\\s*\\}\\s*,?\\s*"
							")+"
						"\\]"
					")"
				"\\s*\\}\\s*"
			")?\\s*,?\\s*"
			"("
				"\"share\"\\s*:\\s*"
				"\\s*\\{\\s*"
					"(\"share_type\"\\s*:\\s*\"(public|vo|pair)\")\\s*,\\s*"
					"(\"share_id\"\\s*:\\s*(null|\".+\"))\\s*,\\s*"
					"(\"in\"\\s*:\\s*\\d+)\\s*,\\s*"
					"(\"out\"\\s*:\\s*\\d+)\\s*,\\s*"
					"(\"policy\"\\s*:\\s*\"(shared|exclusive)\")"
				"\\s*\\}\\s*"
			")?"
		"\\s*\\}\\s*"
		;

const string ConfigurationHandler::get_name_exp =
		"\\s*\"(.+)\"\\s*:\\s*.+\\s*"
		;

const string ConfigurationHandler::get_bool_exp =
		"\\s*\".+\"\\s*:\\s*(true|false)\\s*"
		;

const string ConfigurationHandler::get_num_exp =
		"\\s*\".+\"\\s*:\\s*(\\d+)\\s*"
		;

const string ConfigurationHandler::get_str_exp =
		"\\s*\\{?\\s*\".+\"\\s*:\\s*((null)|\"(.+)\")\\s*\\}?\\s*"
		;

const string ConfigurationHandler::get_vec_exp =
		"\\s*\".+\"\\s*:\\s*\\[(.+)\\]\\s*"
		;

const string ConfigurationHandler::null_str = "null";

const string ConfigurationHandler::SE_TYPE = "se";
const string ConfigurationHandler::GROUP_TYPE = "group";

const string ConfigurationHandler::PUBLIC_SHARE = "public";
const string ConfigurationHandler::VO_SHARE = "vo";
const string ConfigurationHandler::PAIR_SHARE = "pair";


const string ConfigurationHandler::ProtocolParameters::BANDWIDTH = "bandwidth";
const string ConfigurationHandler::ProtocolParameters::NOSTREAMS = "nostreams";
const string ConfigurationHandler::ProtocolParameters::TCP_BUFFER_SIZE = "tcp_buffer_size";
const string ConfigurationHandler::ProtocolParameters::NOMINAL_THROUGHPUT = "nominal_throughput";
const string ConfigurationHandler::ProtocolParameters::BLOCKSIZE = "blocksize";
const string ConfigurationHandler::ProtocolParameters::HTTP_TO = "http_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_PUT_TO = "urlcopy_put_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_PUTDONE_TO = "urlcopy_putdone_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_GET_TO = "urlcopy_get_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_GET_DONETO = "urlcopy_get_doneto";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_TX_TO = "urlcopy_tx_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_TXMARKS_TO = "urlcopy_txmarks_to";
const string ConfigurationHandler::ProtocolParameters::URLCOPY_FIRST_TXMARK_TO = "urlcopy_first_txmark_to";
const string ConfigurationHandler::ProtocolParameters::TX_TO_PER_MB = "tx_to_per_mb";
const string ConfigurationHandler::ProtocolParameters::NO_TX_ACTIVITY_TO = "no_tx_activity_to";
const string ConfigurationHandler::ProtocolParameters::PREPARING_FILES_RATIO = "preparing_files_ratio";


ConfigurationHandler::ConfigurationHandler(string dn):
		updateCount(0),
		insertCount(0),
		deleteCount(0),
		debugCount(0),
		dn(dn),
		db (DBSingleton::instance().getDBObjectInstance()),
		cfg_re(cfg_exp),
		get_name_re(get_name_exp),
		get_bool_re(get_bool_exp),
		get_str_re(get_str_exp),
		get_num_re(get_num_exp),
		get_vec_re(get_vec_exp) {
}

void ConfigurationHandler::parse(string configuration) {

	to_lower(configuration);

	smatch what;
	bool matches = regex_match(configuration, what, cfg_re, match_extra);
	if(!matches){
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Wrong configuration format!" << commit;
		throw Err_Custom("Wrong configuration format!");
	}

	// the whole cfg cmd
	all = what[ALL];

	// get the name and type
	name = getString(what[NAME]);
	type = getString(what[TYPE]);

	// handle active state
	string active_str = what[ACTIVE];
	cfgActive = !active_str.empty();
	if (cfgActive) {
		active = getString(active_str);
	}

	// handle members
	string members_str = what[MEMBERS];
	cfgMembers = !members_str.empty();
	if (cfgMembers) {
		members = getVector(members_str);
	}

	// handle protocol parameters
	string protocol_str = what[PROTOCOL];
	cfgProtocolParams = !protocol_str.empty();
	if (cfgProtocolParams) {
		protocol_pair = getString(what[PROTOCOL_PAIR]);
		parameters = getMap(what[PROTOCOL_PARAMETERS]);
	}

	// handle share configuration
	cfgShare = !string(what[SHARE]).empty();
	if (cfgShare) {
		share_type = getString(what[SHARE_TYPE]);
		share_id = getString(what[SHARE_ID]);
		in = getNumber(what[IN]);
		out = getNumber(what[OUT]);
		policy = getString(what[POLICY]);
	}

	// check the following constraints:

	// in case if it is a public share the share_id has to be 'null'
	if (share_type == PUBLIC_SHARE && share_id != null_str)
		throw Err_Custom("The share_id for public share has to be 'null'!");

	// a SE configuration cannot have members
	if (type == SE_TYPE && cfgMembers)
		throw Err_Custom("Wrong configuration format: A SE configuration may not have members entry!");
}

ConfigurationHandler::~ConfigurationHandler() {

}

string ConfigurationHandler::getName(string cfg) {

	if (cfg.empty()) return string();

	static const int VALUE = 1;
	smatch what;
	regex_match(cfg, what, get_name_re, match_extra);

	return what[VALUE];
}

int ConfigurationHandler::getNumber(string cfg) {

	if (cfg.empty()) return 0;

	static const int VALUE = 1;
	smatch what;
	regex_match(cfg, what, get_num_re, match_extra);
	string value = what[VALUE];

	if (value.empty()) return 0;

	return lexical_cast<int>(value);
}

string ConfigurationHandler::getString(string cfg) {

	if (cfg.empty()) return string();

	static const int STR_NULL = 2;
	static const int VALUE = 3;

	smatch what;
	regex_match(cfg, what, get_str_re, match_extra);

	string null = what[STR_NULL];
	if (!null.empty()) {
		return null_str;
	}

	string ret = what[VALUE];
	replace_all(ret, "*", "%");

	return ret;
}

vector<string> ConfigurationHandler::getVector(string cfg) {

	if (cfg.empty()) return vector<string>();

	static const int VALUE = 1;

	smatch what;
	regex_match(cfg, what, get_vec_re, match_extra);
	string tmp = what[VALUE];

	char_separator<char> sep(",\"");
	tokenizer< char_separator<char> > tokens(tmp, sep);
	tokenizer< char_separator<char> >::iterator it;

	vector<string> ret;

	for(it = tokens.begin(); it != tokens.end(); it++) {
		string s = *it;
		replace_all(s, "*", "%");
		ret.push_back(s);
	}

	return ret;
}

map<string, int> ConfigurationHandler::getMap(string cfg) {

	if (cfg.empty()) return map<string, int>();

	static const int VALUE = 1;
	static const int MAP_KEY = 1;
	static const int MAP_VALUE = 2;

	smatch what;
	regex_match(cfg, what, get_vec_re, match_extra);
	string tmp = what[VALUE];

	char_separator<char> sep(",{}");
	tokenizer< char_separator<char> > tokens(tmp, sep);
	tokenizer< char_separator<char> >::iterator it;

	map<string, int> ret;

	for(it = tokens.begin(); it != tokens.end(); it++) {

		string entry = *it;
		string k = getString(entry);

		++it;
		entry = *it;
		int v = getNumber(entry);
		ret[k] = v;
	}

	return ret;
}

int ConfigurationHandler::getParamValue(const string key) {

	map<string, int>::iterator it = parameters.find(key);

	if (it == parameters.end()) {
		return 0;
	}

	return it->second;
}

void ConfigurationHandler::add() {

	if (type == SE_TYPE) {
		addSeConfiguration();
	} else if (type == GROUP_TYPE) {
		addGroupConfiguration();
	}

	if (insertCount) {
		string action = "insert (x" + lexical_cast<string>(insertCount) + ")";
		db->auditConfiguration(dn, all, action);
	}

	if (updateCount) {
		string action = "update (x" + lexical_cast<string>(updateCount) + ")";
		db->auditConfiguration(dn, all, action);
	}
}

set<string> ConfigurationHandler::handleSeName(string name) {

	set<string> matches = db->getAllMatchingSeNames(name);
	// check if it is already in DB
	if (matches.empty()) {
		// make sure it's not a name with a wildmark
		size_t found = name.find("%");
		if (found != string::npos)
			throw Err_Custom("Wildmarks are not allowed for SEs that do not exist in the DB!");
		// if not, add a new SE record to the DB
		db->addSe("", "", "", name, "", "", "", "", "", "", "");
		insertCount++;
		// and update the set
		matches.insert(name);
	}

	return matches;
}

shared_ptr<SeProtocolConfig> ConfigurationHandler::getProtocolConfig(string name, string pair) {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);

	if (type == SE_TYPE) {

		ret->SE_NAME = name;

	} else if (type == GROUP_TYPE) {

		ret->SE_GROUP_NAME = name;
	}

	ret->BANDWIDTH = getParamValue(ProtocolParameters::BANDWIDTH);
	ret->NOSTREAMS = getParamValue(ProtocolParameters::NOSTREAMS);
	ret->TCP_BUFFER_SIZE = getParamValue(ProtocolParameters::TCP_BUFFER_SIZE);
	ret->NOMINAL_THROUGHPUT = getParamValue(ProtocolParameters::NOMINAL_THROUGHPUT);
	ret->BLOCKSIZE = getParamValue(ProtocolParameters::BLOCKSIZE);
	ret->HTTP_TO = getParamValue(ProtocolParameters::HTTP_TO);
	ret->URLCOPY_PUT_TO = getParamValue(ProtocolParameters::URLCOPY_PUT_TO);
	ret->URLCOPY_PUTDONE_TO = getParamValue(ProtocolParameters::URLCOPY_PUTDONE_TO);
	ret->URLCOPY_GET_TO = getParamValue(ProtocolParameters::URLCOPY_GET_TO);
	ret->URLCOPY_GETDONE_TO = getParamValue(ProtocolParameters::URLCOPY_GET_DONETO);
	ret->URLCOPY_TX_TO = getParamValue(ProtocolParameters::URLCOPY_TX_TO);
	ret->URLCOPY_TXMARKS_TO = getParamValue(ProtocolParameters::URLCOPY_TXMARKS_TO);
	ret->TX_TO_PER_MB = getParamValue(ProtocolParameters::TX_TO_PER_MB);
	ret->NO_TX_ACTIVITY_TO = getParamValue(ProtocolParameters::NO_TX_ACTIVITY_TO);
	ret->PREPARING_FILES_RATIO = getParamValue(ProtocolParameters::PREPARING_FILES_RATIO);

	return ret;
}

void ConfigurationHandler::addShareConfiguration(set<string> matchingNames) {

	set<string> matchingPairNames;
	// check if its a pair share
	if (share_type == PAIR_SHARE) {
		// check if the SE (it's a SE because it's only allowed for SEs) is in the DB
		matchingPairNames = handleSeName(share_id);
	}

	// the share_id for the DB
	string id =
			"\"share_type\":\"" +
			share_type +
			"\",\"share_id\":" +
			(share_id == null_str ? null_str : ("\"" + share_id + "\""))
			;

	// the share_value for the DB
	string value =
			"\"in\":" + lexical_cast<string>(in) +
			",\"out\":" + lexical_cast<string>(out) +
			",\"policy\":\"" + policy + "\""
			;

	// loop for each se or se group name that was matching the given patter
	set<string>::iterator n_it;
	for (n_it = matchingNames.begin(); n_it != matchingNames.end(); n_it++) {

		set<string> share_ids;
		vector<SeAndConfig*> seAndConfig;
		vector<SeAndConfig*>::iterator it;

		// check if the 'SeConfig' entry exists in the DB
		db->getAllShareAndConfigWithCritiria(seAndConfig, *n_it, id, type, string());
		if (!seAndConfig.empty()) {
			// the cfg is already in DB
			// due to the wildmarks in share_id there might be more than one share cfgs
			for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {

				SeAndConfig* tmp = *it;
				// add the share_id to the set containing share_ids that are in the DB
				share_ids.insert(tmp->SHARE_ID);

				// if the value is different than the value set by the user it has to be updated
				if (tmp->SHARE_VALUE != value) {
					FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Updating 'SeConfig' record ..." << commit;
					db->updateSeConfig (
							tmp->SE_NAME,
							tmp->SHARE_ID,
							type,
							value
						);
					updateCount++;
					FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The 'SeConfig' record has been updated!" << commit;
				} else {
					// otherwise we are just logging that no modifications were needed
					FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Share configuration found in DB, nothing to do!" << commit;
				}

				delete tmp;
			}

			// for public share there is only one entry so we are done
			// for vo share without wildmarks there is only one entry so we are done
			// for vo share with wildmarks it is not possible to insert new entry into the
			// DB, the only think we can do is to update some entries therefore we are done
			if (share_type == PUBLIC_SHARE || share_type == VO_SHARE) continue;
		}

		// the SE don't have the corresponding cfg entry in the DB
		// if the share cfg is a public share add an entry to the DB
		// if the share cfg is a vo share check first if there's a wildmark in the
		//	VO name, if yes throw an exception, otherwise add an entry to the DB
		// if the share cfg is a pair share and if the pair se name has a wildmark
		//	find all SEs of interested and create the share cfgs for all of them

		if (share_type == PAIR_SHARE) {

			set<string>::iterator pn_it;
			for (pn_it = matchingPairNames.begin(); pn_it != matchingPairNames.end(); pn_it++) {

				// the share_id for the DB
				string id = "\"share_type\":\"pair\",\"share_id\":\"" + *pn_it + "\"";
				// check if the share_id is already in DB
				if (share_ids.count(id)) continue;

				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
				db->addSeConfig(*n_it, id, type, value);
				insertCount++;
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
			}

		} else {

			if (share_type == VO_SHARE) {
				// it's not in the database
				size_t found = share_id.find("%");
				if (found != string::npos)
					throw Err_Custom("A share configuration cannot be created for a VO share with a wildmark!");
			}

			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
			db->addSeConfig(*n_it, id, type, value);
			insertCount++;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
		}
	}
}

void ConfigurationHandler::addActiveStateConfiguration(set<string> matchingNames) {

	set<string>::iterator it;
	if (type == SE_TYPE) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			db->setGroupOrSeState(*it, string(), active);
			updateCount++;
		}
	} else if (type == GROUP_TYPE) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			db->setGroupOrSeState(string(), *it, active);
			updateCount++;
		}
	}
}

void ConfigurationHandler::addGroupConfiguration() {

	// TODO what if we configure things other than members for a group that does not exist?

	set<string> matchingNames;
	// check if it's a pattern or a new SE group name
	if (name.find("%") == string::npos) {
		matchingNames.insert(name);
	} else {
		matchingNames = db->getAllMatchingSeGroupNames(name);
	}
	set<string>::iterator it;

	if (cfgActive) {
		addActiveStateConfiguration(matchingNames);
	}

	if (cfgMembers) {

		// first find all members that match the patterns
		set<string> matchingMemberNames;
		vector<string>::iterator m_it;
		for (m_it = members.begin(); m_it != members.end(); m_it++) {
			set<string> tmp = handleSeName(*m_it);
			matchingMemberNames.insert(tmp.begin(), tmp.end());
		}

		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			// if an old group exist under the same name replace it!
			db->delete_group(*it);
			// add the SEs to the given group
			set<string>::iterator mm_it;
			for (mm_it = matchingMemberNames.begin(); mm_it != matchingMemberNames.end(); mm_it++) {
				// check if the SE is a member of a group
				string gr = db->get_group_name(*mm_it);
				if (gr.empty()) {
					// if not, add it to the group
					db->add_se_to_group(*mm_it, *it);
					insertCount++;

				} else {
					// if its a member of other group throw an exception
					throw Err_Custom (
							"The SE: " + *mm_it + " is already a member of another SE group (" + gr + ")!"
						);
				}
			}
		}
	}

	if (cfgProtocolParams) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			shared_ptr<SeProtocolConfig> cfg = getProtocolConfig(*it);
			if (db->is_group_protocol_exist(*it)) {
				db->update_se_group_protocol_config(cfg.get());
				updateCount++;
			} else {
				db->add_se_group_protocol_config(cfg.get());
				insertCount++;
			}
		}
	}

	if (cfgShare) {
		// add share configuration (common for SE and SE group)
		addShareConfiguration(matchingNames);
	}
}

void ConfigurationHandler::addSeConfiguration() {

	// ensure that the SE is in the DB
	set<string> matchingNames = handleSeName(name);
	set<string>::iterator it;

	if (cfgActive) {
		addActiveStateConfiguration(matchingNames);
	}

	if (cfgProtocolParams) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			// TODO here should be inner for loop looping over pair se if available
			shared_ptr<SeProtocolConfig> cfg = getProtocolConfig(*it);
			if (db->is_se_protocol_exist(*it)) {
				db->update_se_protocol_config(cfg.get());
				updateCount++;
			} else {
				db->add_se_protocol_config(cfg.get());
				insertCount++;
			}
		}
	}

	if (cfgShare) {
		// add share configuration (common for SE and SE group)
		addShareConfiguration(matchingNames);
	}

}

vector<string> ConfigurationHandler::get(string vo, string name) {

	to_lower(vo);
	to_lower(name);

	replace_all(vo, "*", "%");
	replace_all(name, "*", "%");

	// if the name was not provided replace the empty string with a wildmark
	if (name.empty()) {
		name = "%";
	}

	// prepare the share id (if the vo name was not provided it should be empty)
	const string share_id = vo.empty() ? string() : "\"share_type\":\"vo\",\"share_id\":\"" + vo + "\"";

	// check the share configuration both for a SE and a SE group
	vector<SeAndConfig*> seAndConfig;
	db->getAllShareAndConfigWithCritiria(seAndConfig, name, share_id, string(), string());

	vector<string> ret;
	vector<SeAndConfig*>::iterator it_cfg;
	for (it_cfg = seAndConfig.begin(); it_cfg < seAndConfig.end(); it_cfg++) {

		SeAndConfig* cfg = *it_cfg;
		string resp =
			"{"
				"\"name\":\"" + cfg->SE_NAME + "\","
				"\"type\":\"" + cfg->SHARE_TYPE + "\","
				"\"share\":"
				"{"
					+ cfg->SHARE_ID + ","
					+ cfg->SHARE_VALUE +
				"}"
			"}";

		ret.push_back(resp);
		delete (cfg);
	}

	// if we are looking for vo settings that's it!
	if (!vo.empty()) return ret;

	// get all matching names, it can be both SE and SE group names (since the user may give just a wildmark as the name)
	set<string> matchingNames[2];
	matchingNames[0] = db->getAllMatchingSeNames(name);
	matchingNames[1] = db->getAllMatchingSeGroupNames(name);
	set<string>::iterator it;

	// iterate both over SEs and SE groups
	for (int isGroup = 0; isGroup < 2; isGroup++) {
		for (it = matchingNames[isGroup].begin(); it != matchingNames[isGroup].end(); it++) {

			SeProtocolConfig* cfg = 0;
			if (isGroup) {
				if (!db->is_group_protocol_exist(*it)) continue;
				cfg = db->get_group_protocol_config(*it);
			} else {
				if (!db->is_se_protocol_exist(*it)) continue;
				cfg = db->get_se_protocol_config(*it);
			}

			if (cfg->URLCOPY_TX_TO || cfg->NOSTREAMS) {

				string resp =
					"{"
						"\"name\":\"" + *it + "\","
						"\"type\":\"" + (isGroup ? GROUP_TYPE : SE_TYPE) + "\","
						"\"protocol\":"
						"{";
				if (cfg->URLCOPY_TX_TO) {
					resp += "\"urlcopy_tx_to\":" + lexical_cast<string>(cfg->URLCOPY_TX_TO);
					if (cfg->NOSTREAMS)
						resp += ",";
				}

				if (cfg->NOSTREAMS) {
					resp += "\"nostreams\":" + lexical_cast<string>(cfg->NOSTREAMS);
				}

				resp +=
						"}"
					"}";

				ret.push_back(resp);
			}

			delete cfg;
		}
	}

	// if the group (index = 1) set is empty we are done
	if (matchingNames[1].empty()) return ret;

	// otherwise we have to look for group members
	for (it = matchingNames[1].begin(); it != matchingNames[1].end(); it++) {

		string resp =
				"{"
					"\"name\":\"" + *it + "\","
					"\"type\":\"" + GROUP_TYPE + "\","
					"\"members\":["
				;

		vector<string> members = db->get_group_members(*it);
		vector<string>::iterator it_mbr;
		for (it_mbr = members.begin(); it_mbr < members.end(); it_mbr++) {
			resp += "\"" + *it_mbr + "\"";
			if (it_mbr + 1 != members.end()) resp += ",";
		}

		resp +=	"]}";
		ret.push_back(resp);
	}

	return ret;
}

void ConfigurationHandler::del() {

	// handle protocol parameters
	if (cfgProtocolParams) {
		// check if it's a se or se group
		if (type == SE_TYPE) {

			SeProtocolConfig tmp;
			tmp.SE_NAME = name;
			db->delete_se_protocol_config(&tmp);
			deleteCount++;

		} else if (type == GROUP_TYPE) {

			SeProtocolConfig tmp;
			tmp.SE_GROUP_NAME = name;
			db->delete_se_group_protocol_config(&tmp);
			deleteCount++;
		}
	}

	// handle group members
	if (cfgMembers) {
		// this one will be called only if the type is a group
		// otherwise an exception will be thrown during parsing
		db->delete_group(name);
		deleteCount++;
	}

	// handle share configuration
	if (cfgShare) {
		// the share_id for the DB
		string id =
				"\"share_type\":\"" +
				share_type +
				"\",\"share_id\":" +
				(share_id == null_str ? null_str : ("\"" + share_id + "\""))
				;

		db->deleteSeConfig(name, id, type);
		deleteCount++;
	}

	string action = "delete (x" + lexical_cast<string>(deleteCount) + ")";
	db->auditConfiguration(dn, all, action);
}

