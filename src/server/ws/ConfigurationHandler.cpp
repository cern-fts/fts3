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

using namespace fts3::ws;


const string ConfigurationHandler::cfg_exp =
		"\\s*\\{\\s*"
			"(\"name\"\\s*:\\s*\"[a-zA-Z0-9\\.-]+\")\\s*,\\s*"
			"(\"type\"\\s*:\\s*\"(se|group)\")\\s*,?\\s*"
			"(\"members\"\\s*:\\s*\\[(\\s*\\{\\s*\"se\"\\s*:\\s*\"[a-zA-Z0-9\\.-]+\"\\s*\\}\\s*,?\\s*)+\\])?\\s*,?\\s*"
			"("
				"\"parameters\"\\s*:\\s*"
				"\\s*\\{\\s*"
					"(\"bandwidth\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"nostreams\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"tcp_buffer_size\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"nominal_throughput\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"blocksize\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"http_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_put_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_putdone_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_get_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_get_doneto\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_tx_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_txmarks_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"urlcopy_first_txmark_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"tx_to_per_mb\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"no_tx_activity_to\"\\s*:\\s*\\d+)?\\s*,?\\s*"
					"(\"preparing_files_ratio\"\\s*:\\s*\\d+)?\\s*"
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

const string ConfigurationHandler::get_num_exp =
		"\\s*\".+\"\\s*:\\s*(\\d+)\\s*"
		;

const string ConfigurationHandler::get_str_exp =
		"\\s*\\{?\\s*\".+\"\\s*:\\s*((null)|\"(.+)\")\\s*\\}?\\s*"
		;

const string ConfigurationHandler::get_vec_exp =
		"\\s*\".+\"\\s*:\\s*\\[(\\s*\\{\\s*\".+\"\\s*:\\s*\"[a-zA-Z0-9\\.-]+\"\\s*\\}\\s*,?\\s*)+\\]\\s*"
		;

const string ConfigurationHandler::null_str = "null";

const string ConfigurationHandler::SE_TYPE = "se";
const string ConfigurationHandler::GROUP_TYPE = "group";

const string ConfigurationHandler::PUBLIC_SHARE = "public";
const string ConfigurationHandler::VO_SHARE = "vo";
const string ConfigurationHandler::PAIR_SHARE = "pair";


ConfigurationHandler::ConfigurationHandler(string configuration):
		db (DBSingleton::instance().getDBObjectInstance()),
		cfg_re(cfg_exp),
		get_str_re(get_str_exp),
		get_num_re(get_num_exp),
		get_vec_re(get_vec_exp) {

	to_lower(configuration);

	smatch what;
	regex_match(configuration, what, cfg_re, match_extra);

	string all = what[ALL];
	if (all.empty()) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Wrong configuration format!" << commit;
		throw Err_Custom("Wrong configuration format!");
	}

	// get the name and type
	name = getString(what[NAME]);
	type = getString(what[TYPE]);

	// handle members
	string members_str = what[MEMBERS];
	cfgMembers = !members_str.empty();
	if (cfgMembers) {
		members = getVector(members_str);
	}

	// handle protocol parameters
	cfgProtocolParams = !string(what[PROTOCOL_PARAMETERS]).empty();
	if (cfgProtocolParams) {
		bandwidth = getNumber(what[BANDWIDTH]);
		nostreams = getNumber(what[NOSTREAMS]);
		tcp_buffer_size = getNumber(what[TCP_BUFFER_SIZE]);
		nominal_throughput = getNumber(what[NOMINAL_THROUGHPUT]);
		blocksize = getNumber(what[BLOCKSIZE]);
		http_to = getNumber(what[HTTP_TO]);
		urlcopy_put_to = getNumber(what[URLCOPY_PUT_TO]);
		urlcopy_putdone_to = getNumber(what[URLCOPY_GET_DONETO]);
		urlcopy_get_to = getNumber(what[URLCOPY_GET_TO]);
		urlcopy_get_doneto = getNumber(what[URLCOPY_GET_DONETO]);
		urlcopy_tx_to = getNumber(what[URLCOPY_TX_TO]);
		urlcopy_txmarks_to = getNumber(what[URLCOPY_TXMARKS_TO]);
		urlcopy_first_txmark_to = getNumber(what[URLCOPY_FIRST_TXMARK_TO]);
		tx_to_per_mb = getNumber(what[TX_TO_PER_MB]);
		no_tx_activity_to = getNumber(what[NO_TX_ACTIVITY_TO]);
		preparing_files_ratio = getNumber(what[PREPARING_FILES_RATIO]);
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
}

ConfigurationHandler::~ConfigurationHandler() {

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

	char_separator<char> sep(",");
	tokenizer< char_separator<char> > tokens(tmp, sep);
	tokenizer< char_separator<char> >::iterator it;

	vector<string> ret;

	for(it = tokens.begin(); it != tokens.end(); it++) {
		string s = *it;
		ret.push_back(getString(s));
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

void ConfigurationHandler::addSeIfNotExist(string name) {

	Se* se = 0;
	db->getSe(se, name);
	if (!se) {
		// if not, add a new SE record to the DB
		db->addSe("", "", "", name, "", "", "", "", "", "", "");
	} else {
		// otherwise we only care about the memory
		delete se;
	}
}

shared_ptr<SeProtocolConfig> ConfigurationHandler::getProtocolConfig() {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);

	if (type == SE_TYPE) {

		ret->SE_NAME = name;

	} else if (type == GROUP_TYPE) {

		ret->SE_GROUP_NAME = name;
	}

	ret->BANDWIDTH = bandwidth;
	ret->NOSTREAMS = nostreams;
	ret->TCP_BUFFER_SIZE = tcp_buffer_size;
	ret->NOMINAL_THROUGHPUT = nominal_throughput;
	ret->BLOCKSIZE = blocksize;
	ret->HTTP_TO = http_to;
	ret->URLCOPY_PUT_TO = urlcopy_put_to;
	ret->URLCOPY_PUTDONE_TO = urlcopy_putdone_to;
	ret->URLCOPY_GET_TO = urlcopy_get_to;
	ret->URLCOPY_GETDONE_TO = urlcopy_get_doneto;
	ret->URLCOPY_TX_TO = urlcopy_tx_to;
	ret->URLCOPY_TXMARKS_TO = urlcopy_txmarks_to;
	ret->TX_TO_PER_MB = tx_to_per_mb;
	ret->NO_TX_ACTIVITY_TO = no_tx_activity_to;
	ret->PREPARING_FILES_RATIO = preparing_files_ratio;

	return ret;
}

void ConfigurationHandler::addShareConfiguration() {

	// in case if its public share the share_id has to be 'null'
	if (share_type == PUBLIC_SHARE && share_id != null_str)
		throw Err_Custom("The share_id for public share has to be 'null'!");

	// check if its a pair share
	if (share_type == PAIR_SHARE) {
		// in case if its a SE group throw an exception
		if (type == GROUP_TYPE)
			throw Err_Custom("The pair share is only allowed for SEs (not for SE groups)!");

		// check if the SE (it's a SE because it's only allowed for SEs) is in the DB
		addSeIfNotExist(share_id);
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

	vector<SeAndConfig*> seAndConfig;
	vector<SeAndConfig*>::iterator it;

	// check if the 'SeConfig' exists already in DB
	db->getAllSeAndConfigWithCritiria(seAndConfig, name, id, type, value);
	if (!seAndConfig.empty()) {
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Share configuration found in DB, nothing to do!" << commit;

		// since we did the query using primary key (name + id + type) there must ne only one record
		delete *seAndConfig.begin();
		return;
	}

	// check if the 'SeConfig' exists but with different value
	db->getAllSeAndConfigWithCritiria(seAndConfig, name, id, type, string());

	if (seAndConfig.empty()) {
		// it's not in the database
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
		db->addSeConfig(name, id, type, value);
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
	} else {
		// it is already in the database
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Updating 'SeConfig' record ..." << commit;
		DBSingleton::instance().getDBObjectInstance()->updateSeConfig(name, id, type, value);
		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The 'SeConfig' record has been updated!" << commit;

		// since we did the query using primary key (name + id + type) there must ne only one record
		delete *seAndConfig.begin();
	}
}

void ConfigurationHandler::addGroupConfiguration() {

	if (cfgMembers) {
		// we don't care if the group exist, if not it will be created automatically
		// add the SEs to the given group
		vector<string>::iterator it;
		for (it = members.begin(); it < members.end(); it++) {
			// check if the SE exists
			addSeIfNotExist(*it);

			// add the SE to the group
			db->add_se_to_group(*it, name);
		}
	}

	if (cfgProtocolParams) {
		shared_ptr<SeProtocolConfig> cfg = getProtocolConfig();
		if (db->is_group_protocol_exist(name)) {
			db->update_se_group_protocol_config(cfg.get());
		} else {
			db->add_se_group_protocol_config(cfg.get());
		}
	}

	if (cfgShare) {
		// add share configuration (common for SE and SE group)
		addShareConfiguration();
	}
}

void ConfigurationHandler::addSeConfiguration() {

	// check if some members are specified, if yes throw an exception
	if (cfgMembers) {
		FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Wrong configuration format!" << commit;
		throw Err_Custom("Wrong configuration format: A SE configuration may not have members entry!");
	}

	// ensure that the SE is in the DB
	addSeIfNotExist(name);

	if (cfgProtocolParams) {
		shared_ptr<SeProtocolConfig> cfg = getProtocolConfig();
		if (db->is_se_protocol_exist(name)) {
			db->update_se_protocol_config(cfg.get());
		} else {
			db->add_se_protocol_config(cfg.get());
		}
	}

	if (cfgShare) {
		// add share configuration (common for SE and SE group)
		addShareConfiguration();
	}

}

