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
		dn(dn),
		db (DBSingleton::instance().getDBObjectInstance()),
		cfg(0){
}

ConfigurationHandler::~ConfigurationHandler() {

}

void ConfigurationHandler::parse(string configuration) {

	to_lower(configuration);

	CfgParser parser (configuration);

	switch(parser.getCfgType()) {
	case CfgParser::STANDALONE_SE_CFG:
		cfg.reset(
				new StandaloneSeCfg(dn, parser)
			);
		break;
	case CfgParser::STANDALONE_GR_CFG:
		cfg.reset(
				new StandaloneGrCfg(dn, parser)
			);
		break;
	case CfgParser::SE_PAIR_CFG:
		cfg.reset(
				new SePairCfg(dn, parser)
			);
		break;
	case CfgParser::GR_PAIR_CFG:
		cfg.reset(
				new GrPairCfg(dn, parser)
			);
		break;
	case CfgParser::NOT_A_CFG:
	default:
		throw Err_Custom("Wrong configuration format!");
	}
}

void ConfigurationHandler::add() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is adding configuration" << commit;
	cfg->save();
}

vector<string> ConfigurationHandler::get(string name) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

	vector<string> ret;

	if (db->checkGroupExists(name)) {
		cfg.reset(
				new StandaloneGrCfg(dn, name)
			);
	} else {
		cfg.reset(
				new StandaloneSeCfg(dn, name)
			);
	}

	ret.push_back(cfg->json());

	return ret;
}

vector<string> ConfigurationHandler::getPair(string src, string dest) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is querying configuration" << commit;

	vector<string> ret;

	bool grPair = db->checkGroupExists(src) && db->checkGroupExists(dest);
	bool sePair = !db->checkGroupExists(src) && !db->checkGroupExists(dest);

	if (grPair) {
		cfg.reset(
				new GrPairCfg(dn, src, dest)
			);
	} else if (sePair) {
		cfg.reset(
				new SePairCfg(dn, src, dest)
			);
	} else
		throw Err_Custom("The source and destination have to bem either two SEs or two SE groups!");

	ret.push_back(cfg->json());

	return ret;
}

vector<string> ConfigurationHandler::getPair(string symbolic) {

	scoped_ptr< pair<string, string> > p (
			db->getSourceAndDestination(symbolic)
		);

	if (p.get())
		return getPair(p->first, p->second);
	else
		throw Err_Custom("The symbolic name does not exist!");
}

void ConfigurationHandler::del() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;
	cfg->del();
}

