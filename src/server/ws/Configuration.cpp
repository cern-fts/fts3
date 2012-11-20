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

shared_ptr<SeProtocolConfig> Configuration::getProtocolConfig(map<string, int> protocol) {

	shared_ptr<SeProtocolConfig> ret(new SeProtocolConfig);
	// we are relaying here on the fact that if a parameter is not in the map
	// the default value will be returned which is 0
	ret->NOSTREAMS = protocol[Protocol::NOSTREAMS];
	ret->TCP_BUFFER_SIZE = protocol[Protocol::TCP_BUFFER_SIZE];
	ret->URLCOPY_TX_TO = protocol[Protocol::URLCOPY_TX_TO];
	ret->NO_TX_ACTIVITY_TO = protocol[Protocol::NO_TX_ACTIVITY_TO];

	return ret;
}

map<string, int> Configuration::getProtocolMap(shared_ptr<SeProtocolConfig> protocol) {

	map<string, int> ret;

	ret[Protocol::NOSTREAMS] = protocol->NOSTREAMS;
	ret[Protocol::TCP_BUFFER_SIZE] = protocol->TCP_BUFFER_SIZE;
	ret[Protocol::URLCOPY_TX_TO] = protocol->URLCOPY_TX_TO;
	ret[Protocol::NO_TX_ACTIVITY_TO] = protocol->NO_TX_ACTIVITY_TO;

	return ret;
}

void Configuration::addCfg(string symbolic_name, bool active, string source, string destination, pair<string, int> share, map<string, int> protocol) {

	shared_ptr<SeProtocolConfig> pc = getProtocolConfig(protocol);
	pc->symbolicName = symbolic_name;

	scoped_ptr<SeProtocolConfig> curr (
			db->getProtocol(symbolic_name)
		);

	if (curr.get()) {
		// update
		db->updateProtocol(pc.get());
	} else {
		// insert
		db->addProtocol(pc.get());
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

}
}
