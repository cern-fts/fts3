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

	pair<string, string> p = db->getSymbolicName(source, destination);

	if (p.first.empty())
		throw Err_Custom("A configuration for " + source + " - " + destination + " pair does not exist!");

	symbolic_name = p.first;
	active = p.second == "on";

	share = getShareMap(symbolic_name);
	protocol = getProtocolMap(symbolic_name);
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
	ss << "\"" << "share" << "\":" << Configuration::get(share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::get(protocol);

	return ss.str();
}

void PairCfg::save() {

	addSymbolicName(symbolic_name, source, destination, active);

	map<string, int>::iterator it;
	for (it = share.begin(); it != share.end(); it++) {
		addShareCfg(
				destination,
				*it
			);
	}

	addProtocolCfg(symbolic_name, protocol);
}

} /* namespace ws */
} /* namespace fts3 */
