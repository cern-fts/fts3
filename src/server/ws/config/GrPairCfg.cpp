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
}

GrPairCfg::~GrPairCfg() {
}

string GrPairCfg::json() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "source_group" << "\":\"" << source << "\",";
	ss << "\"" << "destination_group" << "\":\"" << destination << "\",";
	ss << PairCfg::json();
	ss << "}";

	return ss.str();
}

void GrPairCfg::save() {
	checkGroup(source);
	checkGroup(destination);
	PairCfg::save();
}

void GrPairCfg::del() {

}

} /* namespace ws */
} /* namespace fts3 */
