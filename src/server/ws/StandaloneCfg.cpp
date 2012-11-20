/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneCfg.h"

#include <sstream>

namespace fts3 {
namespace ws {

StandaloneCfg::StandaloneCfg(CfgParser& parser) : Configuration(parser) {

	active = parser.get<bool>("active");
	in_share = parser.get< map<string, int> >("in.share");
	in_protocol = parser.get< map<string, int> >("in.protocol");
	out_share = parser.get< map<string, int> >("out.share");
	out_protocol = parser.get< map<string, int> >("out.protocol");
}

StandaloneCfg::~StandaloneCfg() {

}

string StandaloneCfg::get() {

	stringstream ss;

	ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
	ss << "\"" << "in" << "\":{";
	ss << "\"" << "share" << "\":" << Configuration::get(in_share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::get(in_protocol);
	ss << "},";
	ss << "\"" << "out" << "\":{";
	ss << "\"" << "share" << "\":" << Configuration::get(out_share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::get(out_protocol);
	ss << "}";

	return ss.str();
}

void StandaloneCfg::save(string name) {

	map<string, int>::iterator it;
	// create  '* -> se' record
	for (it = in_share.begin(); it != in_share.end(); it++) {
		addCfg(
				"*-" + name,
				active,
				"*",
				name,
				*it,
				in_protocol
			);
	}
	// create 'se -> *' record
	for (it = out_share.begin(); it != out_share.end(); it++) {
		addCfg(
				name + "-*",
				active,
				name,
				"*",
				*it,
				out_protocol
			);
	}
}

} /* namespace common */
} /* namespace fts3 */
