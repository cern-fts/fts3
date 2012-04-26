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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 */

#include "ws/gsoap_config_stubs.h"
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


int config::SoapBindingService::setConfiguration
(
    config__Configuration *_configuration,
    struct impl__setConfigurationResponse &response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'setConfiguration' request" << commit;

	const int SHARE_TYPE_INDEX = 1;
	const int SE_NAME_INDEX = 2;
	const int SHARE_ID_INDEX = 3;
	const int SHARE_NULL_INDEX = 5;
	const int SHARE_VAL_INDEX = 6;

	string 	exp_n = 	"\\s*\"\\s*category\\s*\"\\s*:\\s*\"\\s*(.+)\\s*\"\\s*,";
			exp_n +=	"\\s*\"\\s*name\\s*\"\\s*:\\s*\"\\s*(.+)\\s*\"\\s*,";
			exp_n += 	"\\s*\"\\s*shared_id\\s*\"\\s*:\\s*\"\\s*(publicshare|voshare|spacetokenshare)\\s*\"\\s*,";
			exp_n +=	"\\s*\"\\s*value\\s*\"\\s*:\\s*((null)|\"\\s*(.+)\\s*\")\\s*\\s*";

	string 	exp_v = 	"\\s*\"\\s*in\\s*\"\\s*:\\s*(\\d+)\\s*,";
			exp_v += 	"\\s*\"\\s*out\\s*\"\\s*:\\s*(\\d+)\\s*,";
			exp_v +=	"\\s*\"\\s*policy\\s*\"\\s*:\\s*\"\\s*(.+)\\s*\"\\s*";

	regex re_n(exp_n);
	regex re_v(exp_v);
	smatch what;

	vector<string>& name = _configuration->key;
	vector<string>& value = _configuration->value;
	int size = name.size(), pos;

	for(int i = 0; i < size; i++) {

		// parsing name
		pos = name[i].find('{');
		if (pos != string::npos)
			name[i].erase(pos, 1);

		pos = name[i].find('}');
		if (pos != string::npos)
			name[i].erase(pos, 1);

		to_lower(name[i]);
		regex_match(name[i], what, re_n, match_extra);

		string type = what[SHARE_TYPE_INDEX];
		string name = what[SE_NAME_INDEX];

		string id = "{\"shared_id\":" + what[SHARE_ID_INDEX] + "\",\"value\":";
		string tmp = what[SHARE_NULL_INDEX];
		if (tmp.empty()){
			id += "\"" + what[SHARE_VAL_INDEX] + "\"}";
		} else {
			id += "null}";
		}

		// parsing value
		pos = value[i].find('{');
		if (pos != string::npos)
			value[i].erase(pos, 1);

		pos = value[i].find('}');
		if (pos != string::npos)
			value[i].erase(pos, 1);

		to_lower(value[i]);
		regex_match(value[i], what, re_v, match_extra);

		string val = "{\"in\":" + what[1] + ",\"out\":" + what[2] + ",\"policy\":\"" + what[3] + "\"}";
		string id_extended = id + "=" + val;

		vector<SeAndConfig*> seAndConfig;
		vector<SeAndConfig*>::iterator it;

		try {
			// checking if the 'SeConfig' record exist already, if yes there's nothing to do
			DBSingleton::instance().getDBObjectInstance()->getAllSeAndConfigWithCritiria(seAndConfig, name, id_extended, type);

			if (!seAndConfig.empty()) {
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Nothing to do (" << type << ", " << name << ", " << id_extended << ")" << commit;
				for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {
					delete (*it);
				}
				continue;
			}

			// checking if there's same name but with different value
			DBSingleton::instance().getDBObjectInstance()->getAllSeAndConfigWithCritiria(seAndConfig, name, "", type);

			bool found = false;
			for (it = seAndConfig.begin(); it < seAndConfig.end(); it++) {

				tmp = (*it)->SHARE_ID;

				pos = tmp.find("}={");
				tmp.erase(pos + 1);

				if (tmp == id) {
					found = true;
				}

				delete (*it);
			}

			if (!found) {
				// it's not in the database
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
				DBSingleton::instance().getDBObjectInstance()->addSeConfig(name, id_extended, type);
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB ("
													<< type << ", " << name << ", " << id_extended << ")." << commit;
			} else {
				// it is already in the database
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Updating 'SeConfig' record ..." << commit;
				DBSingleton::instance().getDBObjectInstance()->updateSeConfig(name, id_extended, type);
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "The 'SeConfig' record has been updated ("
													<< type << ", " << name << ", " << id_extended << ")." << commit;
			}
		} catch (std::exception& ex) {
			FTS3_COMMON_LOGGER_NEWLOG (ERR) << "A DB Exception has been caught: " << ex.what() << " ("
												<< type << ", " << name << ", " << id_extended << ")" << commit;

			return SOAP_FAULT;
		}
	}

    return SOAP_OK;
}

/* ---------------------------------------------------------------------- */

int config::SoapBindingService::getConfiguration
(
    struct impl__getConfigurationResponse & response
)
{
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Handling 'getConfiguration' request" << commit;

	set<string> types;
	types.insert("se");
	types.insert("se_pair");
	types.insert("site");

	response.configuration = soap_new_config__Configuration(this, -1);
	vector<string>& names = response.configuration->key;
	vector<string>& values =  response.configuration->value;

	vector<SeConfig*> seConfig;
	vector<SeConfig*>::iterator it;

	DBSingleton::instance().getDBObjectInstance()->getAllSeConfigNoCritiria(seConfig);

	int pos;
	string name, value, tmp;

	for (it = seConfig.begin(); it < seConfig.end(); it++) {
		if (types.count((*it)->SHARE_TYPE)) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << (*it)->SHARE_TYPE << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << (*it)->SE_NAME << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << (*it)->SHARE_ID << commit;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "" << commit;

			value = tmp = (*it)->SHARE_ID;
			pos = tmp.find("}={");

			value.erase(0, pos + 2);
			values.push_back(value);

			tmp.erase(pos + 1);
			tmp.erase(0, 1);
			name = "{\"category\":\"" + (*it)->SHARE_TYPE + "\",\"name\":\"" + (*it)->SE_NAME + "\"," + tmp;
			names.push_back(name);
		}

		delete (*it);
	}

    return SOAP_OK;
}

