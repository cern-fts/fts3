/*
 * GrPairCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "GrPairCfg.h"

namespace fts3 {
namespace ws {

GrPairCfg::GrPairCfg(string dn, CfgParser& parser) : PairCfg(dn, parser) {

	source = parser.get<string>("source_group");
	destination = parser.get<string>("destination_group");

	if (notAllowed.count(source) || notAllowed.count(destination))
		throw Err_Custom("The source or destination name is not a valid!");

	if (symbolic_name_opt)
		symbolic_name = *symbolic_name_opt;
	else
		symbolic_name = source + "-" + destination;


	all = json();
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

} /* namespace ws */
} /* namespace fts3 */
