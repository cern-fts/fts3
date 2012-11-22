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
		cfg.reset(
				new StandaloneSeCfg(parser)
			);
		break;
	case CfgParser::STANDALONE_GR_CFG:
		cfg.reset(
				new StandaloneGrCfg(parser)
			);
		break;
	case CfgParser::SE_PAIR_CFG:
		cfg.reset(
				new SePairCfg(parser)
			);
		break;
	case CfgParser::GR_PAIR_CFG:
		cfg.reset(
				new GrPairCfg(parser)
			);
		break;
	case CfgParser::NOT_A_CFG:
	default:
		throw Err_Custom("Wrong configuration format!");
	}
}

void ConfigurationHandler::add() {

	cfg->save();
}

vector<string> ConfigurationHandler::get(string name) {

	vector<string> ret;

	if (db->checkGroupExists(name)) {
		cfg.reset(
				new StandaloneGrCfg(name)
			);
		type = CfgParser::STANDALONE_GR_CFG;
	} else {
		cfg.reset(
				new StandaloneSeCfg(name)
			);
		type = CfgParser::STANDALONE_SE_CFG;
	}

	ret.push_back(cfg->json());

	return ret;
}

vector<string> ConfigurationHandler::getPair(string src, string dest) {

	vector<string> ret;

	bool grPair = db->checkGroupExists(src) && db->checkGroupExists(dest);
	bool sePair = !db->checkGroupExists(src) && !db->checkGroupExists(dest);

	if (grPair) {
		cfg.reset(
				new GrPairCfg(src, dest)
			);
		type = CfgParser::STANDALONE_GR_CFG;
	} else if (sePair) {
		cfg.reset(
				new SePairCfg(src, dest)
			);
		type = CfgParser::STANDALONE_SE_CFG;
	} else
		throw Err_Custom("The source and destination have to bem either two SEs or two SE groups!");

	ret.push_back(cfg->json());

	return ret;
}

vector<string> ConfigurationHandler::getPair(string symbolic) {

	pair<string, string> p = db->getSourceAndDestination(symbolic);
	return getPair(p.first, p.second);
}

void ConfigurationHandler::del() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;
	cfg->del();
	string action = "delete (x" + lexical_cast<string>(deleteCount) + ")";
}

