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

#include "common/CfgParser.h"

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

	// get the SE/SE group name
	optional<string> name = parser.get<string>("name");
	if (name.is_initialized()) {
		this->name = name.get();
	} else {
		// name is not optional!!!
		throw Err_Custom("SE/SE group name is obligatory");
	}

	// get the type
	optional<string> type = parser.get<string>("type");
	if (type.is_initialized()) {
		this->type = type.get();
		// make sure the type is either 'se' or 'group'
        if (!CfgBlocks::isValidType(this->type)) {
        	throw Err_Custom("configuration type value may be either 'se' or 'group'");
        }
	} else {
		// type is also obligatory!!!
		throw Err_Custom("configuration type is obligatory");
	}

	// handle active state
	optional<bool> active = parser.get<bool>("active");
	if ( (cfgActive = active.is_initialized()) ) {
		this->active = active.get();
	}

	// handle members
	optional< vector<string> > members = parser.get< vector<string> >("members");
	if ( (cfgMembers = members.is_initialized()) ) {

		if (this->type == CfgBlocks::SE_TYPE)
			throw Err_Custom("the member array may only be specified for a SE group");

		this->members = members.get();
	}

	// handle protocol pair
	optional<string> protocol_pair = parser.get<string>("protocol.pair");
	if ( (cfgProtocolPair = protocol_pair.is_initialized()) ) {
		this->protocol_pair = protocol_pair.get();
	}

	// handle protocol parameters
	optional< map<string, int> > parameters = parser.get< map<string, int> >("protocol.parameters");
	if ( (cfgProtocolParams = parameters.is_initialized()) ){
		this->parameters = parameters.get();
	}

	// handle share configuration:
	optional<string> share_type = parser.get<string>("share.type");
	if ( (cfgShare = share_type.is_initialized()) ) {

		this->share_type = share_type.get();
		// make sure the type is either 'public', 'vo' or 'pair'
        smatch what;
        if (!CfgBlocks::isValidShareType(this->share_type)) {
        	throw Err_Custom("share type value may be either 'public', 'vo' or 'pair'");
        }

		optional<string> id = parser.get<string>("share.id");

		if (this->share_type != CfgBlocks::PUBLIC_SHARE_TYPE && !id.is_initialized()) {
			throw Err_Custom("the share.id has to be specified for " + this->share_type);
		}

		if (id.is_initialized()) {
			if (this->share_type == CfgBlocks::PUBLIC_SHARE_TYPE )
				throw Err_Custom("the share.id should not be specified for public-share");

			this->id = id.get();
		}

		optional<int> in = parser.get<int>("share.in");
		if (!in.is_initialized())
			throw Err_Custom("the share.in has to be specified");

		this->in = in.get();

		optional<int> out = parser.get<int>("share.out");
		if (!out.is_initialized())
			throw Err_Custom("the share.out has to be specified");

		this->out = out.get();

		optional<string> policy = parser.get<string>("share.policy");
		if (!policy.is_initialized())
			throw Err_Custom("the share.policy has to be specified");

		this->policy = policy.get();

		// make sure the policy is either 'shared' or 'exclusive'
        if (!CfgBlocks::isValidPolicy(this->policy)) {
        	throw Err_Custom("share policy value may be either 'shared' or 'exclusive'");
        }
	}

	// handle wildcards
	replacer(this->name);
	replacer(this->protocol_pair);
	replacer(this->id);

	for_each(this->members.begin(), this->members.end(), replacer);
}

ConfigurationHandler::~ConfigurationHandler() {

}

void ConfigurationHandler::add() {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is setting a configuration for ";

	if (type == CfgBlocks::SE_TYPE) {

		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "SE: " << name << commit;
		addSeConfiguration();

	} else if (type == CfgBlocks::GROUP_TYPE) {

		FTS3_COMMON_LOGGER_NEWLOG (INFO) << "SE group: " << name << commit;
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
		// make sure it's not a name with a wildcard
		size_t found = name.find(CfgBlocks::SQL_WILDCARD);
		if (found != string::npos)
			throw Err_Custom("Wildcards are not allowed for SEs that do not exist in the DB!");
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

	if (type == CfgBlocks::SE_TYPE) {

		ret->SE_NAME = name;

	} else if (type == CfgBlocks::GROUP_TYPE) {

		ret->SE_GROUP_NAME = name;
	}

	// we are relaying here on the fact that if a parameter is not in the map
	// the default value will be returned which is 0
	ret->BANDWIDTH = parameters[ProtocolParameters::BANDWIDTH];
	ret->NOSTREAMS = parameters[ProtocolParameters::NOSTREAMS];
	ret->TCP_BUFFER_SIZE = parameters[ProtocolParameters::TCP_BUFFER_SIZE];
	ret->NOMINAL_THROUGHPUT = parameters[ProtocolParameters::NOMINAL_THROUGHPUT];
	ret->BLOCKSIZE = parameters[ProtocolParameters::BLOCKSIZE];
	ret->HTTP_TO = parameters[ProtocolParameters::HTTP_TO];
	ret->URLCOPY_PUT_TO = parameters[ProtocolParameters::URLCOPY_PUT_TO];
	ret->URLCOPY_PUTDONE_TO = parameters[ProtocolParameters::URLCOPY_PUTDONE_TO];
	ret->URLCOPY_GET_TO = parameters[ProtocolParameters::URLCOPY_GET_TO];
	ret->URLCOPY_GETDONE_TO = parameters[ProtocolParameters::URLCOPY_GET_DONETO];
	ret->URLCOPY_TX_TO = parameters[ProtocolParameters::URLCOPY_TX_TO];
	ret->URLCOPY_TXMARKS_TO = parameters[ProtocolParameters::URLCOPY_TXMARKS_TO];
	ret->TX_TO_PER_MB = parameters[ProtocolParameters::TX_TO_PER_MB];
	ret->NO_TX_ACTIVITY_TO = parameters[ProtocolParameters::NO_TX_ACTIVITY_TO];
	ret->PREPARING_FILES_RATIO = parameters[ProtocolParameters::PREPARING_FILES_RATIO];

	return ret;
}

void ConfigurationHandler::addShareConfiguration(set<string> matchingNames) {

	set<string> matchingPairNames;
	// check if its a pair share
	if (share_type == CfgBlocks::PAIR_SHARE_TYPE) {
		// check if the SE (it's a SE because it's only allowed for SEs) is in the DB
		matchingPairNames = handleSeName(id);
	}

	// the share_id for the DB
	string share_id = CfgBlocks::shareId(share_type, id);

	// the share_value for the DB
	string value = CfgBlocks::shareValue(
			lexical_cast<string>(in),
			lexical_cast<string>(out),
			policy
		);

	// loop for each se or se group name that was matching the given patter
	set<string>::iterator n_it;
	for (n_it = matchingNames.begin(); n_it != matchingNames.end(); n_it++) {

		set<string> share_ids;
		vector<SeAndConfig*> seAndConfig;
		vector<SeAndConfig*>::iterator it;

		// check if the 'SeConfig' entry exists in the DB
		db->getAllShareAndConfigWithCritiria(seAndConfig, *n_it, share_id, type, string());
		if (!seAndConfig.empty()) {
			// the cfg is already in DB
			// due to the wildcards in share_id there might be more than one share cfgs
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
			// for vo share without wildcards there is only one entry so we are done
			// for vo share with wildcards it is not possible to insert new entry into the
			// DB, the only thing we can do is to update some entries therefore we are done
			if (share_type == CfgBlocks::PUBLIC_SHARE_TYPE || share_type == CfgBlocks::VO_SHARE_TYPE) continue;
		}

		// the SE don't have the corresponding cfg entry in the DB
		// if the share cfg is a public share add an entry to the DB
		// if the share cfg is a vo share check first if there's a wildcard in the
		//	VO name, if yes throw an exception, otherwise add an entry to the DB
		// if the share cfg is a pair share and if the pair se name has a wildcard
		//	find all SEs of interested and create the share cfgs for all of them

		if (share_type == CfgBlocks::PAIR_SHARE_TYPE) {

			set<string>::iterator pn_it;
			for (pn_it = matchingPairNames.begin(); pn_it != matchingPairNames.end(); pn_it++) {

				// the share_id for the DB
				string id = CfgBlocks::pairShare(*pn_it);
				// check if the share_id is already in DB
				if (share_ids.count(id)) continue;

				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
				db->addSeConfig(*n_it, id, type, value);
				insertCount++;
				FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
			}

		} else {

			if (share_type == CfgBlocks::VO_SHARE_TYPE) {
				// it's not in the database
				size_t found = share_id.find(CfgBlocks::SQL_WILDCARD);
				if (found != string::npos)
					throw Err_Custom("A share configuration cannot be created for a VO share with a wildcard!");
			}

			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "Adding new 'SeConfig' record to the DB ..." << commit;
			db->addSeConfig(*n_it, share_id, type, value);
			insertCount++;
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << "New 'SeConfig' record has been added to the DB!" << commit;
		}
	}
}

void ConfigurationHandler::addActiveStateConfiguration(set<string> matchingNames) {

	set<string>::iterator it;
	if (type == CfgBlocks::SE_TYPE) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			db->setGroupOrSeState(*it, string(), str(active));
			updateCount++;
		}
	} else if (type == CfgBlocks::GROUP_TYPE) {
		for (it = matchingNames.begin(); it != matchingNames.end(); it++) {
			db->setGroupOrSeState(string(), *it, str(active));
			updateCount++;
		}
	}
}

void ConfigurationHandler::addGroupConfiguration() {

	// TODO what if we configure things other than members for a group that does not exist?

	set<string> matchingNames;
	// check if it's a pattern or a new SE group name
	if (name.find(CfgBlocks::SQL_WILDCARD) == string::npos) {
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


// TODO create CfgBuilder !!!
vector<string> ConfigurationHandler::get(string vo, string name) {

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is getting configuration";
	if (!vo.empty() || !name.empty()) {

		FTS3_COMMON_LOGGER_NEWLOG (INFO) << " for";

		if (!vo.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << " VO: " << vo;
		}

		if (!name.empty()) {
			FTS3_COMMON_LOGGER_NEWLOG (INFO) << " name: " << name;
		}
	}
	FTS3_COMMON_LOGGER_NEWLOG (INFO) << commit;

	to_lower(vo);
	to_lower(name);

	replacer(vo);
	replacer(name);

	// if the name was not provided replace the empty string with a wildcard
	if (name.empty()) {
		name = CfgBlocks::SQL_WILDCARD;
	}

	// prepare the share id (if the vo name was not provided it should be empty)
	const string share_id = vo.empty() ? string() : CfgBlocks::voShare(vo);

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

	// get all matching names, it can be both SE and SE group names (since the user may give just a wildcard as the name)
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
						"\"type\":\"" + (isGroup ? CfgBlocks::GROUP_TYPE : CfgBlocks::SE_TYPE) + "\","
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
					"\"type\":\"" + CfgBlocks::GROUP_TYPE + "\","
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

	FTS3_COMMON_LOGGER_NEWLOG (INFO) << "DN: " << dn << " is deleting configuration" << commit;

	// handle protocol parameters
	if (cfgProtocolParams) {
		// check if it's a se or se group
		if (type == CfgBlocks::SE_TYPE) {

			SeProtocolConfig tmp;
			tmp.SE_NAME = name;
			db->delete_se_protocol_config(&tmp);
			deleteCount++;

		} else if (type == CfgBlocks::GROUP_TYPE) {

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
		string share_id = CfgBlocks::shareId(share_type, id);

		db->deleteSeConfig(name, share_id, type);
		deleteCount++;
	}

	string action = "delete (x" + lexical_cast<string>(deleteCount) + ")";
	db->auditConfiguration(dn, all, action);
}

string ConfigurationHandler::str(bool b) {
	stringstream ss;
	ss << boolalpha << b;
	return ss.str();
}

