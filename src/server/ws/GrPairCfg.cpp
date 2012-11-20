/*
 * GrPairCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "GrPairCfg.h"

namespace fts3 {
namespace ws {

GrPairCfg::GrPairCfg(CfgParser& parser) : PairCfg(parser) {

	source = parser.get<string>("source_group");
	destination = parser.get<string>("destination_group");

	if (symbolic_name_opt)
		symbolic_name = *symbolic_name_opt;
	else
		symbolic_name = source + "-" + destination;

	checkGroup(source);
	checkGroup(destination);
}

GrPairCfg::~GrPairCfg() {
}

string GrPairCfg::get() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "source_group" << "\":\"" << source << "\",";
	ss << "\"" << "destination_group" << "\":\"" << destination << "\",";
	ss << PairCfg::get();
	ss << "}";

	return ss.str();
}

void GrPairCfg::del() {

}

void GrPairCfg::checkGroup(string group) {
	// check if the group exists
	if (!db->checkGroupExists(group)) {
		throw Err_Custom(
				"The group: " +  group + " does not exist!"
			);
	}
}

} /* namespace ws */
} /* namespace fts3 */
