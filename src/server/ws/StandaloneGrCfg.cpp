/*
 * StandaloneGrCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneGrCfg.h"

namespace fts3 {
namespace ws {

StandaloneGrCfg::StandaloneGrCfg(CfgParser& parser) : StandaloneCfg(parser) {
	group = parser.get<string>("group");
	members = parser.get< vector<string> >("members");

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

StandaloneGrCfg::~StandaloneGrCfg() {
}

string StandaloneGrCfg::get() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "group" << "\":\"" << group << "\",";
	ss << "\"" << "members" << "\":" << Configuration::get(members) << ",";
	ss << StandaloneCfg::get();
	ss << "}";

	return ss.str();
}

void StandaloneGrCfg::save() {

	StandaloneCfg::save(group);
}

void StandaloneGrCfg::del() {

}

} /* namespace common */
} /* namespace fts3 */
