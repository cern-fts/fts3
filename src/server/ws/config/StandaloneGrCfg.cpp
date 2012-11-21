/*
 * StandaloneGrCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneGrCfg.h"

namespace fts3 {
namespace ws {

StandaloneGrCfg::StandaloneGrCfg(string name) : group(name) {

	if (!db->checkGroupExists(group))
		throw Err_Custom("The SE group: " + group + " does not exist!");

	active = true; // TODO so far it is not possible to set the active state for a group to 'off' (false)

	// init shares and protocols
	init(name);

	// get group members
	db->getGroupMembers(name, members);
}

StandaloneGrCfg::StandaloneGrCfg(CfgParser& parser) : StandaloneCfg(parser) {
	group = parser.get<string>("group");
	members = parser.get< vector<string> >("members");
}

StandaloneGrCfg::~StandaloneGrCfg() {
}

string StandaloneGrCfg::json() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "group" << "\":\"" << group << "\",";
	ss << "\"" << "members" << "\":" << Configuration::get(members) << ",";
	ss << StandaloneCfg::json();
	ss << "}";

	return ss.str();
}

void StandaloneGrCfg::save() {

	addGroup(group, members);
	StandaloneCfg::save(group);
}

void StandaloneGrCfg::del() {

}

} /* namespace common */
} /* namespace fts3 */
