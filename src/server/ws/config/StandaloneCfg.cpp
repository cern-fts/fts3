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

StandaloneCfg::StandaloneCfg(string dn, CfgParser& parser) : Configuration(dn) {

	active = parser.get<bool>("active");
	in_share = parser.get< map<string, int> >("in.share");
	in_protocol = parser.get< map<string, int> >("in.protocol");
	out_share = parser.get< map<string, int> >("out.share");
	out_protocol = parser.get< map<string, int> >("out.protocol");
}

StandaloneCfg::~StandaloneCfg() {

}

void StandaloneCfg::init(string name) {
	// get SE in and out share
	in_share = getShareMap(any, name);
	out_share = getShareMap(name, any);
	// get SE in and out protocol
	in_protocol = getProtocolMap(any, name);
	out_protocol = getProtocolMap(name, any);
}

string StandaloneCfg::json() {

	stringstream ss;

	ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
	ss << "\"" << "in" << "\":{";
	ss << "\"" << "share" << "\":" << Configuration::json(in_share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::json(in_protocol);
	ss << "},";
	ss << "\"" << "out" << "\":{";
	ss << "\"" << "share" << "\":" << Configuration::json(out_share) << ",";
	ss << "\"" << "protocol" << "\":" << Configuration::json(out_protocol);
	ss << "}";

	return ss.str();
}

void StandaloneCfg::save(string name) {

	// add the in-link
	addLinkCfg(any, name, active, any + "-" + name, in_protocol);
	// add the shares for the in-link
	addShareCfg(any, name, in_share);

	// add the out-link
	addLinkCfg(name, any, active, name + "-" + any, out_protocol);
	// add the shares for out-link
	addShareCfg(name, any, out_share);
}

void StandaloneCfg::del(string name) {

	// delete the shares for the in-link
	delShareCfg(any, name);
	// delete the in-link
	delLinkCfg(any, name);

	// delete the shares for the out-link
	delShareCfg(name, any);
	// delete the out-link
	delLinkCfg(name, any);
}

} /* namespace common */
} /* namespace fts3 */
