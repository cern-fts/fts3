/*
 * SePairCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "SePairCfg.h"

#include <sstream>

namespace fts3 {
namespace ws {

SePairCfg::SePairCfg(string dn, CfgParser& parser) : PairCfg(dn, parser) {

	source = parser.get<string>("source_se");
	destination = parser.get<string>("destination_se");

	if (notAllowed.count(source) || notAllowed.count(destination))
		throw Err_Custom("The source or destination name is not a valid!");

	if (symbolic_name_opt)
		symbolic_name = *symbolic_name_opt;
	else
		symbolic_name = source + "-" + destination;

	all = json();
}

SePairCfg::~SePairCfg() {
}

string SePairCfg::json() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "source_se" << "\":\"" << source << "\",";
	ss << "\"" << "destination_se" << "\":\"" << destination << "\",";
	ss << PairCfg::json();
	ss << "}";

	return ss.str();
}

void SePairCfg::save() {
	addSe(source);
	addSe(destination);
	PairCfg::save();
}

} /* namespace ws */
} /* namespace fts3 */
