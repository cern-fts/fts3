/*
 * PairCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "PairCfg.h"

#include <sstream>

namespace fts3 {
namespace ws {

PairCfg::PairCfg(CfgParser& parser) : Configuration(parser) {

	symbolic_name = parser.get<string>("symbolic_name");
	share = parser.get< map<string, int> >("share");
	protocol = parser.get< map<string, int> >("protocol");
	active = parser.get<bool>("active");
}

PairCfg::~PairCfg() {
}

string PairCfg::get() {

	stringstream ss;

	ss << "\"" << "symbolic_name" << "\":\"" << symbolic_name << "\",";
	ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
	ss << "\"" << "share" << "\":" << Configuration::get(share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::get(protocol);

	return ss.str();
}

void PairCfg::save() {

	map<string, int>::iterator it;
	for (it = share.begin(); it != share.end(); it++) {
		addCfg(
				symbolic_name,
				active,
				source,
				destination,
				*it,
				protocol
			);
	}
}

} /* namespace ws */
} /* namespace fts3 */
