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

SePairCfg::SePairCfg(CfgParser& parser) : PairCfg(parser) {

	source = parser.get<string>("source_se");
	destination = parser.get<string>("destination_se");

	if (symbolic_name_opt)
		symbolic_name = *symbolic_name_opt;
	else
		symbolic_name = source + "-" + destination;

	addSe(source);
	addSe(destination);
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

void SePairCfg::del() {

}

} /* namespace ws */
} /* namespace fts3 */
