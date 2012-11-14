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

#include <sstream>
#include <algorithm>

#include <boost/optional.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

using namespace fts3::ws;


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
		db (DBSingleton::instance().getDBObjectInstance()) {
}

void ConfigurationHandler::parse(string configuration) {

	to_lower(configuration);

	CfgParser parser (configuration);

	// the whole cfg cmd
	all = configuration;

	// configuration type
	type = parser.getCfgType();

	switch(type) {
	case CfgParser::GROUP_MEMBERS_CFG: {
		// get the SE/SE group name
		name = parser.get<string>("name");
		if (!name) throw Err_Custom("The group name is required!");
		// get the member SEs
		members = parser.get< vector<string> >("members");
		if (!members) throw Err_Custom("The members of the group are required!");
		// check if the group exists
		if (db->checkGroupExists(*name)) {
			// if an old group exist under the same name replace it!
			db->delete_group(*name);
			deleteCount++;
		}
		vector<string>::iterator it;
		for (it = (*members).begin(); it != (*members).end(); it++) {
			//check if SE exists
			checkSe(*it);
			// check if the SE is a member of a group
			string gr = db->get_group_name(*it);
			if (gr.empty()) {
				// if not, add it to the group
				db->addGroupMember(*name, *it);
				insertCount++;

			} else {
				// if its a member of other group throw an exception
				throw Err_Custom (
						"The SE: " + *it + " is already a member of another SE group (" + gr + ")!"
					);
			}
		}
		break;
	}
	case CfgParser::TRANSFER_CFG:
		// configuration name
		cfg_name = parser.get<string>("config_name");
		if (!cfg_name)  throw Err_Custom("The symbolic name of the configuration has to be specified!");
		to_lower(*cfg_name);
		// get the source se
		source = parser.getSource();
		if (!source) throw Err_Custom("The source has to be specified!");
		// get the destination se
		destination = parser.get<string>("destination");
		if (!destination) throw Err_Custom("The destination has to be specified!");
		// get active transfers
		active_transfers = parser.get<int>("active_transfers");
		// get vo
		vo = parser.get<string>("vo");
		if (!vo || !active_transfers)
			throw Err_Custom("Both the 'active transfers' and the 'vo' have to be defined!");
		to_lower(*vo);
		// get the protocol parameters
		protocol = parser.get< map<string, int> >("protocol");
		// the configuration has to have at least protocol or share (both are also allowed)
		if (!protocol) throw Err_Custom("The protocol parameters have to be specified!");
		// do standard checks
		checkEntity(*source);
		checkEntity((*destination));
		break;
	case CfgParser::SE_TRANSFER_CFG:
		// get the configuration symbolic name
		cfg_name = parser.get<string>("config_name");
		if (!cfg_name)  throw Err_Custom("The symbolic name of the configuration has to be specified!");
		to_lower(*cfg_name);
		// get the group name
		name = parser.get<string>("group");
		if (!name) throw Err_Custom("The name of the group has to be specified!");
		checkGroup(*name);
		// get the group member name
		member = parser.get<string>("se");
		if (!member) throw Err_Custom("The 'se' has to be specified!");
		checkSe(*member);
		active_transfers = parser.get<int>("active_transfers");
		if (!active_transfers) throw Err_Custom("The number of 'active_transfers' has to be specified!");
		break;
	default:
		throw Err_Custom("Wrong configuration format!");
	}
}

ConfigurationHandler::~ConfigurationHandler() {

}

void ConfigurationHandler::checkSe(string name) {
	//check if SE exists
	Se* ptr = 0;
	db->getSe(ptr, name);
	if (!ptr) {
		// if not add it to the DB
		db->addSe(string(), string(), string(), name, string(), string(), string(), string(), string(), string(), string());
		insertCount++;
	} else
		delete ptr;
}

void ConfigurationHandler::checkGroup(string name) {
	// check if the group exists
	// TODO if the group does not exist check BDII and import it if it's there
	if (!db->checkGroupExists(name)) {
		throw Err_Custom(
				"The group: " +  name + " does not exist!"
			);
	}
}

void ConfigurationHandler::checkEntity(tuple<string, bool> ent) {
	if (ent.get<1>()) {
		// source is a group
		checkGroup(ent.get<0>());
	} else {
		// source is a se
		checkSe(ent.get<0>());
	}
}

void ConfigurationHandler::add() {

	switch (type) {
	case CfgParser::TRANSFER_CFG:
		addTransferConfiguration();
		break;
	case CfgParser::SE_TRANSFER_CFG:
		addSeTransferConfiguration();
		break;
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

shared_ptr<SeProtocolConfig> ConfigurationHandler::getProtocolConfig() {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);
	// we are relaying here on the fact that if a parameter is not in the map
	// the default value will be returned which is 0
	ret->NOSTREAMS = (*protocol)[ProtocolParameters::NOSTREAMS];
	ret->TCP_BUFFER_SIZE = (*protocol)[ProtocolParameters::TCP_BUFFER_SIZE];
	ret->URLCOPY_TX_TO = (*protocol)[ProtocolParameters::URLCOPY_TX_TO];
	ret->NO_TX_ACTIVITY_TO = (*protocol)[ProtocolParameters::NO_TX_ACTIVITY_TO];

	return ret;
}

void ConfigurationHandler::addTransferConfiguration() {

	shared_ptr<SeProtocolConfig> pc = getProtocolConfig();
	string src = source.get().get<0>();
	string dest = destination.get().get<0>();
	int id = db->getProtocolIdFromConfig(src, dest, *vo);
	if (id) {
		// update
		db->updateProtocol(pc.get(), id);
	} else {
		// insert
		id = db->addProtocol(pc.get());
	}

	bool update = true;
	scoped_ptr<SeConfig> cfg (
			db->getConfig(src, dest, *vo)
		);

	if (!cfg.get()) {
		cfg.reset(new SeConfig);
		update = false;
	}

	cfg->active = *active_transfers;
	cfg->destination = dest;
	cfg->protocolId = id;
	cfg->source = src;
	cfg->symbolicName = *cfg_name;
	cfg->vo = *vo;

	if (update)
		// update
		db->updateConfig(cfg.get());
	else
		// insert
		db->addNewConfig(cfg.get());
}

void ConfigurationHandler::addSeTransferConfiguration() {

	bool update = true;
	scoped_ptr<SeGroup> sg (
			db->getGroupConfig(*cfg_name, *name, *member)
		);

	if (!sg.get()) {
		sg.reset(new SeGroup);
		update = false;
	}

	sg->active = *active_transfers;
	sg->groupName = *name;
	sg->member = *member;
	sg->symbolicName = *cfg_name;

	if (update)
		// update
		db->updateGroupConfig(sg.get());
	else
		// insert
		db->addGroupConfig(sg.get());
}

string ConfigurationHandler::buildPairCfg(SeConfig* cfg, SeProtocolConfig* prot) {

	stringstream ss;
	ss << "{";

	ss << "\"" << "config_name" << "\":\"" << cfg->symbolicName << "\",";



	if (db->checkGroupExists(cfg->source)) {
		ss << "\"" << "source_group" << "\":\"" << cfg->source << "\",";
	} else {
		ss << "\"" << "source_se" << "\":\"" << cfg->source << "\",";
	}

	if (db->checkGroupExists(cfg->destination)) {
		ss << "\"" << "destination_group" << "\":\"" << cfg->destination << "\",";
	} else {
		ss << "\"" << "destination_se" << "\":\"" << cfg->destination << "\",";
	}

	ss << "\"" << "active_transfers" << "\":" << cfg->active << ",";
	ss << "\"" << "vo" << "\":\"" << cfg->vo << "\",";

	ss << "\"" << "protocol" << "\":[";
	ss << "{\"" << ProtocolParameters::NOSTREAMS << "\":" << prot->NOSTREAMS << "},";
	ss << "{\"" << ProtocolParameters::URLCOPY_TX_TO << "\":" << prot->URLCOPY_TX_TO << "}";
	ss << "]";
	ss << "}";

	return ss.str();
}

string ConfigurationHandler::buildGroupCfg(string name, vector<string> members) {

	stringstream ss;

	ss << "{";
	ss << "\"" << "group" << "\":\"" << name << "\",";
	ss << "\"" << "members" << "\":[";

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end();) {
		ss << "\"" << *it << "\"";
		it++;
		if (it != members.end()) ss << ",";
	}

	ss << "]";
	ss << "}";

	return ss.str();
}

string ConfigurationHandler::buildSeCfg(SeGroup* sg) {

	stringstream ss;

	ss << "{";
	ss << "\"" << "config_name" << "\":\"" << sg->symbolicName << "\",";
	ss << "\"" << "group" << "\":\"" << sg->groupName << "\",";
	ss << "\"" << "se" << "\":\"" << sg->member << "\",";
	ss << "\"" << "active_transfers" << "\":" << sg->active;
	ss << "}";

	return ss.str();
}

vector<string> ConfigurationHandler::getGroupCfg(string cfg_name, string name) {

	vector<string> ret;

	vector<string> members;
	db->getGroupMembers(name, members);
	ret.push_back(
			buildGroupCfg(name, members)
		);

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		scoped_ptr<SeGroup> sg (
					db->getGroupConfig(cfg_name, name, *it)
				);
		ret.push_back(
				buildSeCfg(sg.get())
			);

	}

	return ret;
}

// TODO create CfgBuilder !!!
vector<string> ConfigurationHandler::get(string cfg_name, string vo) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is getting configuration";
	if (!vo.empty() || !cfg_name.empty()) {

		FTS3_COMMON_LOGGER_NEWLOG (INFO) << " for";

		if (!vo.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << " VO: " << vo;
		}

		if (!cfg_name.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "symbolic name: " << name;
		}
	}
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;

	to_lower(vo);
	to_lower(cfg_name);

	vector<string> ret;

	scoped_ptr<SeConfig> cfg (
			db->getConfig(cfg_name, vo)
		);
	if (!cfg.get()) {
		throw Err_Custom(
				"A configuration for symbolic name: "
				+ cfg_name +
				(vo.empty() ? "" : " and for vo: " + vo)
				+ " does not exist!"
			);
	}

	scoped_ptr<SeProtocolConfig> prot (
			db->getProtocol(cfg->protocolId)
		);

	// create the pair configuration
	ret.push_back(
			buildPairCfg(cfg.get(), prot.get())
		);

	if (db->checkGroupExists(cfg->source)) {
		vector<string> gr = getGroupCfg(cfg_name, cfg->source);
		ret.insert(ret.end(), gr.begin(), gr.end());
	}

	if (db->checkGroupExists(cfg->destination)) {
		vector<string> gr = getGroupCfg(cfg_name, cfg->destination);
		ret.insert(ret.end(), gr.begin(), gr.end());
	}

	return ret;
}

void ConfigurationHandler::del() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;

	switch (type) {
	case CfgParser::SE_TRANSFER_CFG: {
		scoped_ptr<SeGroup> sg (new SeGroup);
		sg->active = *active_transfers;
		sg->groupName = *name;
		sg->member = *member;
		sg->symbolicName = *cfg_name;
		db->deleteGroupConfig(sg.get());
		deleteCount++;
		break;
	}
	case CfgParser::TRANSFER_CFG: {
		scoped_ptr<SeConfig> sc (
				db->getConfig(source.get().get<0>(), destination.get().get<0>(), *vo)
			);
		int id = sc->protocolId;
		db->deleteConfig(sc.get());
		deleteCount++;
		db->deleteProtocol(id);
		deleteCount++;
		break;
	}
	case CfgParser::GROUP_MEMBERS_CFG: {
		db->deleteMembersFromGroup(*name, *members);
		deleteCount++;
		break;
	}
	}

	string action = "delete (x" + lexical_cast<string>(deleteCount) + ")";
	db->auditConfiguration(dn, all, action);
}

string ConfigurationHandler::str(bool b) {
	stringstream ss;
	ss << boolalpha << b;
	return ss.str();
}

