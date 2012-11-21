/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "Configuration.h"
#include "common/error.h"


namespace fts3 {
namespace ws {


const string Configuration::Protocol::BANDWIDTH = "bandwidth";
const string Configuration::Protocol::NOSTREAMS = "nostreams";
const string Configuration::Protocol::TCP_BUFFER_SIZE = "tcp_buffer_size";
const string Configuration::Protocol::NOMINAL_THROUGHPUT = "nominal_throughput";
const string Configuration::Protocol::BLOCKSIZE = "blocksize";
const string Configuration::Protocol::HTTP_TO = "http_to";
const string Configuration::Protocol::URLCOPY_PUT_TO = "urlcopy_put_to";
const string Configuration::Protocol::URLCOPY_PUTDONE_TO = "urlcopy_putdone_to";
const string Configuration::Protocol::URLCOPY_GET_TO = "urlcopy_get_to";
const string Configuration::Protocol::URLCOPY_GET_DONETO = "urlcopy_get_doneto";
const string Configuration::Protocol::URLCOPY_TX_TO = "urlcopy_tx_to";
const string Configuration::Protocol::URLCOPY_TXMARKS_TO = "urlcopy_txmarks_to";
const string Configuration::Protocol::URLCOPY_FIRST_TXMARK_TO = "urlcopy_first_txmark_to";
const string Configuration::Protocol::TX_TO_PER_MB = "tx_to_per_mb";
const string Configuration::Protocol::NO_TX_ACTIVITY_TO = "no_tx_activity_to";
const string Configuration::Protocol::PREPARING_FILES_RATIO = "preparing_files_ratio";



Configuration::Configuration(CfgParser& parser) : db (DBSingleton::instance().getDBObjectInstance()) {

}

string Configuration::get(map<string, int> params) {

	stringstream ss;

	ss << "[";

	map<string, int>::iterator it;
	for (it = params.begin(); it != params.end();) {
		ss << "{\"" << it->first << "\":" << it->second << "}";
		it++;
		if (it != params.end()) ss << ",";
	}

	ss << "]";

	return ss.str();
}

string Configuration::get(vector<string> members) {

	stringstream ss;

	ss << "[";

	vector<string>::iterator it;
	for (it = members.begin(); it != members.end();) {
		ss << "\"" << *it << "\"";
		it++;
		if (it != members.end()) ss << ",";
	}

	ss << "]";

	return ss.str();
}

void Configuration::addSe(string se, bool active) {
	//check if SE exists
	Se* ptr = 0;
	db->getSe(ptr, se);
	if (!ptr) {
		// if not add it to the DB
		db->addSe(string(), string(), string(), se, active ? "on" : "off", string(), string(), string(), string(), string(), string());
	} else
		delete ptr;
}

void Configuration::addGroup(string group, vector<string>& members) {
	vector<string>::iterator it;
	for (it = members.begin(); it != members.end(); it++) {
		addSe(*it);
	}

	if (db->checkGroupExists(group)) {
		// if the group exists remove it!
		vector<string> tmp;
		db->getGroupMembers(group, tmp);
		db->deleteMembersFromGroup(group, tmp);
	}

	db->addMemberToGroup(group, members);
}

void Configuration::checkGroup(string group) {
	// check if the group exists
	if (!db->checkGroupExists(group)) {
		throw Err_Custom(
				"The group: " +  group + " does not exist!"
			);
	}
}

void Configuration::addLinkCfg(string source, string destination, bool active, string symbolic_name, map<string, int>& protocol) {

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	bool update = true;
	if (!cfg.get()) {
		cfg.reset(new LinkConfig);
		update = false;
	}

	cfg->source = source;
	cfg->destination = destination;
	cfg->state = active ? "on" : "off";
	cfg->symbolic_name = symbolic_name;

	cfg->NOSTREAMS = protocol[Protocol::NOSTREAMS];
	cfg->TCP_BUFFER_SIZE = protocol[Protocol::TCP_BUFFER_SIZE];
	cfg->URLCOPY_TX_TO = protocol[Protocol::URLCOPY_TX_TO];
	cfg->NO_TX_ACTIVITY_TO = protocol[Protocol::NO_TX_ACTIVITY_TO];

	if (update) {
		db->updateLinkConfig(cfg.get());
	} else {
		db->addLinkConfig(cfg.get());
	}

}

void Configuration::addShareCfg(string source, string destination, map<string, int>& share) {

	map<string, int>::iterator it;
	for (it = share.begin(); it != share.end(); it++) {

		scoped_ptr<ShareConfig> cfg (
				db->getShareConfig(source, destination, it->first)
			);

		bool update = true;
		if (!cfg.get()) {
			cfg.reset(new ShareConfig);
			update = false;
		}

		cfg->source = source;
		cfg->destination = destination;
		cfg->vo = it->first;
		cfg->active_transfers = it->second;

		if (update) {
			db->updateShareConfig(cfg.get());
		} else {
			db->addShareConfig(cfg.get());
		}
	}
}

map<string, int> Configuration::getProtocolMap(string source, string destination) {

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	return getProtocolMap(cfg.get());
}

map<string, int> Configuration::getProtocolMap(LinkConfig* cfg) {

	map<string, int> ret;
	if (cfg->NOSTREAMS)
		ret[Protocol::NOSTREAMS] = cfg->NOSTREAMS;
	if (cfg->TCP_BUFFER_SIZE)
		ret[Protocol::TCP_BUFFER_SIZE] = cfg->TCP_BUFFER_SIZE;
	if (cfg->URLCOPY_TX_TO)
		ret[Protocol::URLCOPY_TX_TO] = cfg->URLCOPY_TX_TO;
	if (cfg->NO_TX_ACTIVITY_TO)
		ret[Protocol::NO_TX_ACTIVITY_TO] = cfg->NO_TX_ACTIVITY_TO;

	return ret;
}

map<string, int> Configuration::getShareMap(string source, string destination) {

	vector<ShareConfig*> vec = db->getShareConfig(source, destination);

	if (vec.empty()) {
		throw Err_Custom(
				"A configuration for source: '" + source + "' and destination: '" + destination + "' does not exist!"
			);
	}

	map<string, int> ret;

	vector<ShareConfig*>::iterator it;
	for (it = vec.begin(); it != vec.end(); it++) {
		scoped_ptr<ShareConfig> cfg(*it);
		ret[cfg->vo] = cfg->active_transfers;
	}

	return ret;
}

}
}
