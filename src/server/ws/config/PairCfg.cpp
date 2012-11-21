/*
 * PairCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "PairCfg.h"

#include <sstream>

#include <utility>

namespace fts3 {
namespace ws {

PairCfg::PairCfg(string source, string destination) : source(source), destination(destination) {

	scoped_ptr<LinkConfig> cfg (
			db->getLinkConfig(source, destination)
		);

	if (!cfg.get())
		throw Err_Custom("A configuration for " + source + " - " + destination + " pair does not exist!");

	symbolic_name = cfg->symbolic_name;
	active = cfg->state == "on";

	share = getShareMap(source, destination);
	protocol = getProtocolMap(cfg.get());
}

PairCfg::PairCfg(CfgParser& parser) : Configuration(parser) {

	symbolic_name_opt = parser.get_opt("symbolic_name");
	share = parser.get< map<string, int> >("share");
	protocol = parser.get< map<string, int> >("protocol");
	active = parser.get<bool>("active");
}

PairCfg::~PairCfg() {
}

string PairCfg::json() {

	stringstream ss;

	ss << "\"" << "symbolic_name" << "\":\"" << symbolic_name << "\",";
	ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
	ss << "\"" << "share" << "\":" << Configuration::json(share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::json(protocol);

	return ss.str();
}

void PairCfg::save() {
	// add link
	addLinkCfg(source, destination, active, symbolic_name, protocol);
	// add shres for the link
	addShareCfg(source, destination, share);
}

void PairCfg::del() {
	// delete shares
	delShareCfg(source, destination);
	// delete the link itself
	delLinkCfg(source, destination);
}

} /* namespace ws */
} /* namespace fts3 */
