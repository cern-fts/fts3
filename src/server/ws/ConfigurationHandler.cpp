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
 *      Author: simonm
 */

#include "ConfigurationHandler.h"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/assign.hpp>

using namespace fts3::ws;
using namespace boost::assign;

const string ConfigurationHandler::cfg_exp =
		"\\s*\\{\\s*"
			"(\"name\"\\s*:\\s*\"[\\*a-zA-Z0-9\\.-]+\")\\s*,\\s*"
			"(\"type\"\\s*:\\s*\"(se|group)\")\\s*,?\\s*"
			"(\"members\"\\s*:\\s*\\[(\\s*\"[\\*a-zA-Z0-9\\.-]+\"\\s*,?\\s*)+\\])?\\s*,?\\s*"
			"("
				"\"protocol\"\\s*:\\s*"
				"\\s*\\{\\s*"
					"(\"[a-zA-Z0-9\\.-_]+\"\\s*:\\s*\\d+\\s*,?\\s*)+"
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

const string ConfigurationHandler::get_num_exp =
		"\\s*\".+\"\\s*:\\s*(\\d+)\\s*"
		;

const string ConfigurationHandler::get_str_exp =
		"\\s*\\{?\\s*\".+\"\\s*:\\s*((null)|\"(.+)\")\\s*\\}?\\s*"
		;

const string ConfigurationHandler::get_vec_exp =
		"\\s*\".+\"\\s*:\\s*\\[((\\s*\"[\\*a-zA-Z0-9\\.-]+\"\\s*,?\\s*)+)\\]\\s*"
		;

const string ConfigurationHandler::get_par_exp =
		"\\s*\".+\"\\s*:\\s*\\{((\\s*\".+\"\\s*:\\s*\\d+\\s*,?)+)\\}\\s*"
		;

const string ConfigurationHandler::null_str = "null";

const string ConfigurationHandler::SE_TYPE = "se";
const string ConfigurationHandler::GROUP_TYPE = "group";

const string ConfigurationHandler::PUBLIC_SHARE = "public";
const string ConfigurationHandler::VO_SHARE = "vo";
const string ConfigurationHandler::PAIR_SHARE = "pair";


ConfigurationHandler::ConfigurationHandler():
		db (DBSingleton::instance().getDBObjectInstance()),
		cfg_re(cfg_exp),
		get_name_re(get_name_exp),
		get_str_re(get_str_exp),
		get_num_re(get_num_exp),
		get_vec_re(get_vec_exp),
		get_par_re(get_par_exp),
		parameterNameToId (map_list_of
			("bandwidth", BANDWIDTH)
			("nostreams", NOSTREAMS)
			("tcp_buffer_size", TCP_BUFFER_SIZE)
			("nominal_throughput", NOMINAL_THROUGHPUT)
			("blocksize", BLOCKSIZE)
			("http_to", HTTP_TO)
			("urlcopy_put_to", URLCOPY_PUT_TO)
			("urlcopy_putdone_to", URLCOPY_PUTDONE_TO)
			("urlcopy_get_to", URLCOPY_GET_TO)
			("urlcopy_get_doneto", URLCOPY_GET_DONETO)
			("urlcopy_tx_to", URLCOPY_TX_TO)
			("urlcopy_txmarks_to", URLCOPY_TXMARKS_TO)
			("urlcopy_first_txmark_to", URLCOPY_FIRST_TXMARK_TO)
			("tx_to_per_mb", TX_TO_PER_MB)
			("no_tx_activity_to", NO_TX_ACTIVITY_TO)
			("preparing_files_ratio", PREPARING_FILES_RATIO).to_container(parameterNameToId)
		) {
}

void ConfigurationHandler::parse(string configuration) {

	to_lower(configuration);

	smatch what;
	bool matches = regex_match(configuration, what, cfg_re, match_extra);
	if(!matches){
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Wrong configuration format!" << commit;
		throw Err_Custom("Wrong configuration format!");
	}

	// get the name and type
	name = getString(what[NAME]);
	replace_all(name, "*", "%");
	type = getString(what[TYPE]);

	// handle members
	string members_str = what[MEMBERS];
	cfgMembers = !members_str.empty();
	if (cfgMembers) {
		members = getVector(members_str);
	}

	// handle protocol parameters
	string params_str = what[PROTOCOL_PARAMETERS];
	cfgProtocolParams = !params_str.empty();
	if (cfgProtocolParams) {
		parameters = getParamVector(params_str);
	}

	// handle share configuration
	cfgShare = !string(what[SHARE]).empty();
	if (cfgShare) {
		share_type = getString(what[SHARE_TYPE]);
		share_id = getString(what[SHARE_ID]);
		replace_all(share_id, "*", "%");
		in = getNumber(what[IN]);
		out = getNumber(what[OUT]);
		policy = getString(what[POLICY]);
	}
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

	return what[VALUE];
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
		ret.push_back(s);
	}

	return ret;
}

vector<int> ConfigurationHandler::getParamVector(string cfg) {

	if (cfg.empty()) return vector<int>();

	static const int VALUE = 1;

	smatch what;
	regex_match(cfg, what, get_par_re, match_extra);
	string tmp = what[VALUE];

	char_separator<char> sep(",");
	tokenizer< char_separator<char> > tokens(tmp, sep);
	tokenizer< char_separator<char> >::iterator it;

	vector<int> ret(PARAMETERS_NMB, 0);

	for(it = tokens.begin(); it != tokens.end(); it++) {
		string s = *it;
		map<string, ProtocolParameters>::const_iterator id = parameterNameToId.find (
				getName(s)
			);
		if (id != parameterNameToId.end()) {
			ret[id->second] = (getNumber(s));
		}

	}

	return ret;
}

void ConfigurationHandler::add() {

	if (type == SE_TYPE) {
		addSeConfiguration();
	} else if (type == GROUP_TYPE) {
		addGroupConfiguration();
	}
}

// TODO change the method name, now it not only adds the se to the db if it does not exist
// it also checks if its a se name pattern with a wildmark, in this case it returns all se
// name that matches the pattern, if there are no ses that match the pattern an exception
// is thrown
set<string> ConfigurationHandler::addSeIfNotExist(string name) {

	set<string> matches = db->getAllMatchingSeNames(name);
	// check if it is already in DB
	if (matches.empty()) {
		// make sure it's not a name with a wildmark
		size_t found = name.find("%");
		if (found != string::npos)
			throw Err_Custom("Wildmarks are not allowed for SEs that do not exist in the DB!");
		// if not, add a new SE record to the DB
		db->addSe("", "", "", name, "", "", "", "", "", "", "");
		// and update the set
		matches.insert(name);
	}

	return matches;
}

shared_ptr<SeProtocolConfig> ConfigurationHandler::getProtocolConfig(string name) {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);

	if (type == SE_TYPE) {

		ret->SE_NAME = name;

	} else if (type == GROUP_TYPE) {

		ret->SE_GROUP_NAME = name;
	}

	ret->BANDWIDTH = parameters[BANDWIDTH];
	ret->NOSTREAMS = parameters[NOSTREAMS];
	ret->TCP_BUFFER_SIZE = parameters[TCP_BUFFER_SIZE];
	ret->NOMINAL_THROUGHPUT = parameters[NOMINAL_THROUGHPUT];
	ret->BLOCKSIZE = parameters[BLOCKSIZE];
	ret->HTTP_TO = parameters[HTTP_TO];
	ret->URLCOPY_PUT_TO = parameters[URLCOPY_PUT_TO];
	ret->URLCOPY_PUTDONE_TO = parameters[URLCOPY_PUTDONE_TO];
	ret->URLCOPY_GET_TO = parameters[URLCOPY_GET_TO];
	ret->URLCOPY_GETDONE_TO = parameters[URLCOPY_GET_DONETO];
	ret->URLCOPY_TX_TO = parameters[URLCOPY_TX_TO];
	ret->URLCOPY_TXMARKS_TO = parameters[URLCOPY_TXMARKS_TO];
	ret->TX_TO_PER_MB = parameters[TX_TO_PER_MB];
	ret->NO_TX_ACTIVITY_TO = parameters[NO_TX_ACTIVITY_TO];
	ret->PREPARING_FILES_RATIO = parameters[PREPARING_FILES_RATIO];

	return ret;
}

void ConfigurationHandler::addShareConfiguration(set<string> matchingNames) {

	// in case if its public share the share_id has to be 'null'
	if (share_type == PUBLIC_SHARE && share_id != null_str)
		throw Err_Custom("The share_id for public share has to be 'null'!");

	set<string> matchingPairNames;
	// check if its a pair share
	if (share_type == PAIR_SHARE) {
		// in case if its a SE group throw an exception
		if (type == GROUP_TYPE)
			throw Err_Custom("The pair share is only allowed for SEs (not for SE groups)!");

		// check if the SE (it's a SE because it's only allowed for SEs) is in the DB
		matchingPairNames = addSeIfNotExist(share_id);
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
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
		}
	}
}

void ConfigurationHandler::addGroupConfiguration() {

	set<string> matchingNames;
	// check if it's a pattern or a new SE group name
	if (name.find("%") == string::npos) {
		matchingNames.insert(name);
	} else {
		matchingNames = db->getAllMatchingSeNames(name);
	}
	set<string>::iterator it;

	if (cfgMembers) {

		// first find all members that match the patterns
		set<string> matchingMemberNames;
		vector<string>::iterator m_it;
		for (m_it = members.begin(); m_it != members.end(); m_it++) {
			set<string> tmp = addSeIfNotExist(*m_it);
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
					db->add_se_to_group(*mm_it, name);
				} else /*if (gr != name)*/ { // TODO check now when the group is deleted this check should be unnecessary
					// if its a member of other group throw an exception
					throw Err_Custom (
							"The SE: " + *it + " is already a member of another SE group (" + gr + ")!"
						);
				}
			}
		}
	}

	if (cfgProtocolParams) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			shared_ptr<SeProtocolConfig> cfg = getProtocolConfig(*it);
			if (db->is_group_protocol_exist(name)) {
				db->update_se_group_protocol_config(cfg.get());
			} else {
				db->add_se_group_protocol_config(cfg.get());
			}
		}
	}

	if (cfgShare) {
		// add share configuration (common for SE and SE group)
		addShareConfiguration(matchingNames);
	}
}

void ConfigurationHandler::addSeConfiguration() {

	// check if some members are specified, if yes throw an exception
	if (cfgMembers) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Wrong configuration format!" << commit;
		throw Err_Custom("Wrong configuration format: A SE configuration may not have members entry!");
	}

	// ensure that the SE is in the DB
	set<string> matchingNames = addSeIfNotExist(name);
	set<string>::iterator it;

	if (cfgProtocolParams) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			shared_ptr<SeProtocolConfig> cfg = getProtocolConfig(*it);
			if (db->is_se_protocol_exist(*it)) {
				db->update_se_protocol_config(cfg.get());
			} else {
				db->add_se_protocol_config(cfg.get());
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

	const string share_id = "\"share_type\":\"vo\",\"share_id\":\"" + vo + "\"";

	vector<SeConfig*> seConfig;
	db->getAllShareConfigNoCritiria(seConfig);

	vector<string> ret;
	vector<SeConfig*>::iterator it_cfg;
	for (it_cfg = seConfig.begin(); it_cfg < seConfig.end(); it_cfg++) {

		SeConfig* cfg = *it_cfg;
		
		if (!vo.empty()) {
			if (cfg->SHARE_ID != share_id) continue;
		}

		if (!name.empty()) {
			if (cfg->SE_NAME != name) continue;
		}

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

	// we are not interested in protocol specific configuration if vo filter was used
	if (vo.empty()) {

		vector<Se*> se;
		db->getAllSeInfoNoCritiria(se);

		vector<Se*>::iterator it_se;
		for (it_se = se.begin(); it_se < se.end(); it_se++) {

			string se_name = (*it_se)->NAME;

			if (!name.empty()) {
				if (se_name != name) continue;
			}

			if (!db->is_se_protocol_exist(se_name)) {
				delete *it_se;
				continue;
			}

			SeProtocolConfig* cfg = db->get_se_protocol_config(se_name);
			if (cfg->URLCOPY_TX_TO || cfg->NOSTREAMS) {

				string resp =
					"{"
						"\"name\":\"" + se_name + "\","
						"\"type\":\"" + SE_TYPE + "\","
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
			delete *it_se;
		}
	}

	// if we are looking for vo settings the group specific info is ommited
	if (!vo.empty()) return ret;

	vector<string> groups = db->get_group_names();
	vector<string>::iterator it_gr;
	for (it_gr = groups.begin(); it_gr < groups.end(); it_gr++) {

		if (!name.empty()) {
			if (name != *it_gr) continue;
		}

		string resp =
				"{"
					"\"name\":\"" + *it_gr + "\","
					"\"type\":\"" + GROUP_TYPE + "\","
					"\"members\":["
				;

		vector<string> members = db->get_group_members(*it_gr);
		vector<string>::iterator it_mbr;
		for (it_mbr = members.begin(); it_mbr < members.end(); it_mbr++) {
			resp += "\"" + *it_mbr + "\"";
			if (it_mbr + 1 != members.end()) resp += ",";
		}

		resp +=	"]}";
		ret.push_back(resp);

		if (db->is_group_protocol_exist(*it_gr)) {
			SeProtocolConfig* cfg = db->get_group_protocol_config(*it_gr);

			resp.erase();
			resp +=
					"{"
						"\"name\":\"" + *it_gr + "\","
						"\"type\":\"" + GROUP_TYPE + "\","
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
			delete cfg;
		}
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

		} else if (type == GROUP_TYPE) {

			SeProtocolConfig tmp;
			tmp.SE_NAME = name;
			db->delete_se_group_protocol_config(&tmp);
		}
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
	}
}

