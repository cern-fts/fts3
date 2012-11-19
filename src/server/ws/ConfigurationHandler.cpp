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
	case CfgParser::STANDALONE_SE_CFG:
		seCfg.se = parser.get<string>("se");
		seCfg.active = parser.get<bool>("active");
		seCfg.in_share = parser.get< map<string, int> >("in.share");
		seCfg.in_protocol = parser.get< map<string, int> >("in.protocol");
		seCfg.out_share = parser.get< map<string, int> >("out.share");
		seCfg.out_protocol = parser.get< map<string, int> >("out.protocol");

		handleSe(seCfg.se, seCfg.active);

		break;
	case CfgParser::STANDALONE_GR_CFG:
		grCfg.group = parser.get<string>("group");
		grCfg.members = parser.get< vector<string> >("members");
		grCfg.active = parser.get<bool>("active");
		grCfg.in_share = parser.get< map<string, int> >("in.share");
		grCfg.in_protocol = parser.get< map<string, int> >("in.protocol");
		grCfg.out_share = parser.get< map<string, int> >("out.share");
		grCfg.out_protocol = parser.get< map<string, int> >("out.protocol");

		handleGrMember(grCfg.group, grCfg.members);

		break;
	case CfgParser::SE_PAIR_CFG:
		pairCfg.symbolic_name = parser.get<string>("symbolic_name");
		pairCfg.source = parser.get<string>("source_se");
		pairCfg.destination = parser.get<string>("destination_se");
		pairCfg.share = parser.get< map<string, int> >("share");
		pairCfg.protocol = parser.get< map<string, int> >("protocol");
		pairCfg.active = parser.get<bool>("active");

		handleSe(pairCfg.source);
		handleSe(pairCfg.destination);

		break;
	case CfgParser::GR_PAIR_CFG:
		pairCfg.symbolic_name = parser.get<string>("symbolic_name");
		pairCfg.source = parser.get<string>("source_group");
		pairCfg.destination = parser.get<string>("destination_group");
		pairCfg.share = parser.get< map<string, int> >("share");
		pairCfg.protocol = parser.get< map<string, int> >("protocol");
		pairCfg.active = parser.get<bool>("active");

		handleGroup(pairCfg.source);
		handleGroup(pairCfg.destination);

		break;
	default:
		throw Err_Custom("Wrong configuration format!");
	}
}

ConfigurationHandler::~ConfigurationHandler() {

}

void ConfigurationHandler::handleSe(string name, bool active) {

	//check if SE exists
	Se* ptr = 0;
	db->getSe(ptr, name);
	if (!ptr) {
		// if not add it to the DB
		db->addSe(string(), string(), string(), name, active ? "on" : "off", string(), string(), string(), string(), string(), string());
		insertCount++;
	} else
		delete ptr;
}

void ConfigurationHandler::handleGroup(string name) {
	// check if the group exists
	if (!db->checkGroupExists(name)) {
		throw Err_Custom(
				"The group: " +  name + " does not exist!"
			);
	}
}

void ConfigurationHandler::handleGrMember(string gr, vector<string> members) {

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		handleSe(*it);
	}

	db->addMemberToGroup(gr, members);
}

void ConfigurationHandler::add() {

	switch (type) {
	case CfgParser::STANDALONE_SE_CFG:
		addStandaloneSeCfg();
		break;
	case CfgParser::STANDALONE_GR_CFG:
		addStandaloneGrCfg();
		break;
	case CfgParser::SE_PAIR_CFG:
	case CfgParser::GR_PAIR_CFG:
		addPairCfg();
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

void ConfigurationHandler::addStandaloneSeCfg() {

	map<string, int>::iterator it;
	// create  '* -> se' record
	for (it = seCfg.in_share.begin(); it != seCfg.in_share.end(); it++) {
		addCfg(
				"*-" + seCfg.se,
				seCfg.active,
				"*",
				seCfg.se,
				*it,
				seCfg.in_protocol
			);
	}
	// create 'se -> *' record
	for (it = seCfg.out_share.begin(); it != seCfg.out_share.end(); it++) {
		addCfg(
				seCfg.se + "-*",
				seCfg.active,
				seCfg.se,
				"*",
				*it,
				seCfg.out_protocol
			);
	}
}

void ConfigurationHandler::addStandaloneGrCfg() {

	map<string, int>::iterator it;
	// create  '* -> group' record
	for (it = grCfg.in_share.begin(); it != grCfg.in_share.end(); it++) {
		addCfg(
				"*-" + grCfg.group,
				grCfg.active,
				"*",
				grCfg.group,
				*it,
				grCfg.in_protocol
			);
	}
	// create 'group -> *' record
	for (it = grCfg.out_share.begin(); it != grCfg.out_share.end(); it++) {
		addCfg(
				grCfg.group + "-*",
				grCfg.active,
				grCfg.group,
				"*",
				*it,
				grCfg.out_protocol
			);
	}
}

void ConfigurationHandler::addPairCfg() {

	map<string, int>::iterator it;
	for (it = pairCfg.share.begin(); it != pairCfg.share.end(); it++) {
		addCfg(
				pairCfg.symbolic_name,
				pairCfg.active,
				pairCfg.source,
				pairCfg.destination,
				*it,
				pairCfg.protocol
			);
	}
}

void ConfigurationHandler::addCfg(string symbolic_name, bool active, string source, string destination, pair<string, int> share, map<string, int> protocol) {

	shared_ptr<SeProtocolConfig> pc = getProtocolConfig(protocol);
	int id = db->getProtocolIdFromConfig(source, destination, share.first); // TODO remove protocol per vo!
	if (id) {
		// update
		db->updateProtocol(pc.get(), id);
	} else {
		// insert
		id = db->addProtocol(pc.get());
	}

	bool update = true;
	vector<SeConfig*> v = db->getConfig(source, destination, share.first);
	scoped_ptr<SeConfig> cfg (
			v.empty() ? 0 : v.front()
		);

	if (!cfg.get()) {
		cfg.reset(new SeConfig);
		update = false;
	}

	cfg->active = share.second;
	cfg->destination = destination;
	cfg->protocolId = id;
	cfg->source = source;
	cfg->symbolicName = symbolic_name;
	cfg->vo = share.first;
	cfg->state = active ? "on" : "off";

	if (update) {
		// update
		db->updateConfig(cfg.get());
	} else {
		// insert
		db->addNewConfig(cfg.get());
	}
}

shared_ptr<SeProtocolConfig> ConfigurationHandler::getProtocolConfig(map<string, int> protocol) {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);
	// we are relaying here on the fact that if a parameter is not in the map
	// the default value will be returned which is 0
	ret->NOSTREAMS = protocol[ProtocolParameters::NOSTREAMS];
	ret->TCP_BUFFER_SIZE = protocol[ProtocolParameters::TCP_BUFFER_SIZE];
	ret->URLCOPY_TX_TO = protocol[ProtocolParameters::URLCOPY_TX_TO];
	ret->NO_TX_ACTIVITY_TO = protocol[ProtocolParameters::NO_TX_ACTIVITY_TO];

	return ret;
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
	ss << "],";
	ss << "\"" << "state" << "\":\"" << cfg->state << "\"";
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
	ss << "\"" << "vo" << "\":\"" << sg->vo << "\",";
	ss << "\"" << "active_transfers" << "\":" << sg->active;
	ss << "}";

	return ss.str();
}

vector<string> ConfigurationHandler::getGroupCfg(string cfg_name, string name, string vo) {

	vector<string> ret;

//	vector<string> members;
//	db->getGroupMembers(name, members);
//	ret.push_back(
//			buildGroupCfg(name, members)
//		);
//
//	vector<string>::iterator it;
//	for (it = members.begin(); it != members.end(); it++) {
//		scoped_ptr<SeGroup> sg (
//					db->getGroupConfig(cfg_name, name, *it, vo)
//				);
//		if (sg.get())
//			ret.push_back(
//					buildSeCfg(sg.get())
//				);
//
//	}

	return ret;
}

vector<string> ConfigurationHandler::doGet(SeConfig* cfg) {

//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is getting configuration";
//	if (!cfg->vo.empty() || !cfg->symbolicName.empty()) {
//
//		FTS3_COMMON_LOGGER_NEWLOG (INFO) << " for";
//
//		if (!cfg->vo.empty()) {
//			FTS3_COMMON_LOGGER_NEWLOG (INFO) << " VO: " << vo;
//		}
//
//		if (!cfg->symbolicName.empty()) {
//			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "symbolic name: " << name;
//		}
//	}
//	FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;

	vector<string> ret;
//
//	scoped_ptr<SeProtocolConfig> prot (
//			db->getProtocol(cfg->protocolId)
//		);
//
//	// create the pair configuration
//	ret.push_back(
//			buildPairCfg(cfg, prot.get())
//		);
//
//	if (db->checkGroupExists(cfg->source)) {
//		vector<string> gr = getGroupCfg(cfg->symbolicName, cfg->source, cfg->vo);
//		ret.insert(ret.end(), gr.begin(), gr.end());
//	}
//
//	if (db->checkGroupExists(cfg->destination)) {
//		vector<string> gr = getGroupCfg(cfg->symbolicName, cfg->destination, cfg->vo);
//		ret.insert(ret.end(), gr.begin(), gr.end());
//	}

	return ret;
}

vector<string> ConfigurationHandler::get(string src, string dest, string vo) {

//	to_lower(vo);
//	vector<SeConfig*> vec = db->getConfig(src, dest, vo);
//
//	if (vec.empty()) {
//		throw Err_Custom(
//				"A configuration for source: " + src  + " and destination: " + dest +
//				(vo.empty() ? "" : " and for vo: " + vo)
//				+ " does not exist!"
//			);
//	}

	vector<string> ret;
//	vector<SeConfig*>::iterator it;
//
//	for (it = vec.begin(); it != vec.end(); it++) {
//		scoped_ptr<SeConfig> cfg (*it);
//		vector<string> v = doGet(cfg.get());
//		ret.insert(ret.end(), v.begin(), v.end());
//	}


	return ret;
}

vector<string> ConfigurationHandler::get(string cfg_name, string vo) {

	to_lower(vo);
	to_lower(cfg_name);

	to_lower(vo);
	vector<SeConfig*> vec = db->getConfig(cfg_name, vo);

	if (vec.empty()) {
		throw Err_Custom(
				"A configuration for symboli name: " + cfg_name +
				(vo.empty() ? "" : " and for vo: " + vo)
				+ " does not exist!"
			);
	}

	vector<string> ret;
	vector<SeConfig*>::iterator it;

	for (it = vec.begin(); it != vec.end(); it++) {
		scoped_ptr<SeConfig> cfg (*it);
		vector<string> v = doGet(cfg.get());
		ret.insert(ret.end(), v.begin(), v.end());
	}

	return ret;
}

void ConfigurationHandler::del() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;

//	switch (type) {
//	case CfgParser::SE_TRANSFER_CFG: {
//		scoped_ptr<SeGroup> sg (new SeGroup);
//		sg->active = *active_transfers;
//		sg->groupName = *name;
//		sg->member = *member;
//		sg->symbolicName = *cfg_name;
//		db->deleteGroupConfig(sg.get());
//		deleteCount++;
//		break;
//	}
//	case CfgParser::TRANSFER_CFG: {
//
//		vector<SeConfig*> v = db->getConfig(source.get().get<0>(), destination.get().get<0>(), *vo);
//		scoped_ptr<SeConfig> sc (
//				v.empty() ? 0 : v.front()
//			);
//		int id = sc->protocolId;
//		db->deleteConfig(sc.get());
//		deleteCount++;
//		db->deleteProtocol(id);
//		deleteCount++;
//		break;
//	}
//	case CfgParser::GROUP_MEMBERS_CFG: {
//		db->deleteMembersFromGroup(*name, *members);
//		deleteCount++;
//		break;
//	}
//	}

	string action = "delete (x" + lexical_cast<string>(deleteCount) + ")";
	db->auditConfiguration(dn, all, action);
}

string ConfigurationHandler::str(bool b) {
	stringstream ss;
	ss << boolalpha << b;
	return ss.str();
}

