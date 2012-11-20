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

#include "StandaloneSeCfg.h"
#include "StandaloneGrCfg.h"
#include "SePairCfg.h"
#include "GrPairCfg.h"

using namespace fts3::ws;

ConfigurationHandler::ConfigurationHandler(string dn):
		updateCount(0),
		insertCount(0),
		deleteCount(0),
		debugCount(0),
		dn(dn),
		db (DBSingleton::instance().getDBObjectInstance()),
		cfg(0){
}

ConfigurationHandler::~ConfigurationHandler() {

	if (cfg) delete cfg;
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
		cfg = new StandaloneSeCfg(parser);
		break;
	case CfgParser::STANDALONE_GR_CFG:
		cfg = new StandaloneGrCfg(parser);
		break;
	case CfgParser::SE_PAIR_CFG:
		cfg = new SePairCfg(parser);
		break;
	case CfgParser::GR_PAIR_CFG:
		cfg = new GrPairCfg(parser);
		break;
	case CfgParser::NOT_A_CFG:
	default:
		throw Err_Custom("Wrong configuration format!");
	}
}

void ConfigurationHandler::add() {

	cfg->save();

	if (insertCount) {
		string action = "insert (x" + lexical_cast<string>(insertCount) + ")";
		db->auditConfiguration(dn, all, action);
	}

	if (updateCount) {
		string action = "update (x" + lexical_cast<string>(updateCount) + ")";
		db->auditConfiguration(dn, all, action);
	}
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

vector<string> ConfigurationHandler::getStandalone(string name) {

	vector<string> ret;

	return ret;
}

vector<string> ConfigurationHandler::getPair(string src, string dest) {

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

vector<string> ConfigurationHandler::getSymbolic(string cfg_name) {

	to_lower(cfg_name);

	vector<SeConfig*> vec = db->getConfig(cfg_name, string());

	if (vec.empty()) {
		throw Err_Custom(
				"A configuration for symboli name: " + cfg_name
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

	cfg->del();

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

