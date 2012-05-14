/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "ws/gsoap_stubs.h"
#include "db/generic/SingleDbInstance.h"
#include "common/logger.h"

#include <vector>
#include <string>
#include <set>
#include <exception>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

using namespace std;
using namespace db;
using namespace fts3::common;
using namespace boost;
using namespace boost::algorithm;


int SoapBindingService::setConfiguration
(
    config__Configuration *_configuration,
    struct implcfg__setConfigurationResponse &response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;

	const int TYPE_INDEX = 1;
	const int NAME_INDEX = 2;
	const int SHARE_TYPE_INDEX = 3;
	const int SHARE_NULL_INDEX = 5;
	const int SHARE_ID_INDEX = 6;
	const int INBOUND_INDEX = 7;
	const int OUTBOUND_INDEX = 8;
	const int POLICY_INDEX = 9;


	string 	exp = 	"\\s*\"\\s*type\\s*\"\\s*:\\s*\"\\s*(se|site)\\s*\"\\s*,"
					"\\s*\"\\s*name\\s*\"\\s*:\\s*\"\\s*(.+)\\s*\"\\s*,"
					"\\s*\"\\s*share_type\\s*\"\\s*:\\s*\"\\s*(public|vo|pair)\\s*\"\\s*,"
					"\\s*\"\\s*share_id\\s*\"\\s*:\\s*((null)|\"\\s*(.+)\\s*\")\\s*,"
					"\\s*\"\\s*in\\s*\"\\s*:\\s*(\\d+)\\s*,"
					"\\s*\"\\s*out\\s*\"\\s*:\\s*(\\d+)\\s*,"
					"\\s*\"\\s*policy\\s*\"\\s*:\\s*\"\\s*(.+)\\s*\"\\s*";

	regex re(exp);
	smatch what;

	vector<string>& cfgs = _configuration->cfg;
	vector<string>::iterator it;
	int pos;

//	for(int i = 0; i < size; i++) {
	for(it = cfgs.begin(); it < cfgs.end(); it++) {

		string cfg = *it;
		// parsing cfg
		pos = cfg.find('{');
		if (pos != string::npos)
			cfg.erase(pos, 1);

		pos = cfg.find('}');
		if (pos != string::npos)
			cfg.erase(pos, 1);

		to_lower(cfg);
		regex_match(cfg, what, re, match_extra);

		string type = what[TYPE_INDEX];
		string name = what[NAME_INDEX];

		string id = "\"share_type\":\"" + what[SHARE_TYPE_INDEX] + "\",\"share_id\":";
		string tmp = what[SHARE_NULL_INDEX];
		if (tmp.empty()){
			id += "\"" + what[SHARE_ID_INDEX] + "\"";
		} else {
			id += "null}";
		}

		string val = "\"in\":" + what[INBOUND_INDEX] +
					",\"out\":" + what[OUTBOUND_INDEX] +
					",\"policy\":\"" + what[POLICY_INDEX] + "\"";

		vector<SeAndConfig*> seAndConfig;
		vector<SeAndConfig*>::iterator it;

		try {
			// checking if the 'SeConfig' record exist already, if yes there's nothing to do
			DBSingleton::instance().getDBObjectInstance()->getAllSeAndConfigWithCritiria(seAndConfig, name, id, type, val);

			if (!seAndConfig.empty()) {
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Nothing to do (" << type << ", " << name << ", " << id << ", " << val << ")" << commit;
				for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {
					delete (*it);
				}
				continue;
			}

			// checking if there's same name but with different value
			DBSingleton::instance().getDBObjectInstance()->getAllSeAndConfigWithCritiria(seAndConfig, name, id, type, "");

			if (seAndConfig.empty()) {
				// it's not in the database
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
				DBSingleton::instance().getDBObjectInstance()->addSeConfig(name, id, type, val);
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB ("
													<< type << ", " << name << ", " << id << ", " << val << ")." << commit;
			} else {
				// it is already in the database
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Updating 'SeConfig' record ..." << commit;
				DBSingleton::instance().getDBObjectInstance()->updateSeConfig(name, id, type, val);
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The 'SeConfig' record has been updated ("
													<< type << ", " << name << ", " << id <<  ", " << val << ")." << commit;

				delete *seAndConfig.begin();
			}
		} catch (std::exception& ex) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A DB Exception has been caught: " << ex.what() << " ("
												<< type << ", " << name << ", " << id << ", " << val << ")" << commit;

			return SOAP_FAULT;
		}
	}

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int SoapBindingService::getConfiguration
(
    struct implcfg__getConfigurationResponse & response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getConfiguration' request" << commit;

	set<string> types;
	types.insert("se");
	types.insert("site");

	response.configuration = soap_new_config__Configuration(this, -1);
	vector<string>& cfgs = response.configuration->cfg;
	vector<SeConfig*> seConfig;
	vector<SeConfig*>::iterator it;

	DBSingleton::instance().getDBObjectInstance()->getAllSeConfigNoCritiria(seConfig);

	int pos;
	string cfg;
	SeConfig* seCfg;

	for (it = seConfig.begin(); it < seConfig.end(); it++) {

		seCfg = *it;

		if (types.count((*it)->SHARE_TYPE)) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << seCfg->SHARE_TYPE << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << seCfg->SE_NAME << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << seCfg->SHARE_ID << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << seCfg->SHARE_VALUE << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;


			cfg = "{"
					"\"type\":\"" + seCfg->SHARE_TYPE + "\","
					"\"name\":\"" + seCfg->SE_NAME + "\","
					+ seCfg->SHARE_ID + ","
					+ seCfg->SHARE_VALUE +
				"}";

			cfgs.push_back(cfg);
		}

		delete (seCfg);
	}

    return SOAP_OK;
}

