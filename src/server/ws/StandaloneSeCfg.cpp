/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneSeCfg.h"

#include <sstream>

namespace fts3 {
namespace ws {

StandaloneSeCfg::StandaloneSeCfg(CfgParser& parser) : StandaloneCfg(parser)  {
	se = parser.get<string>("se");
	addSe(se);
}

StandaloneSeCfg::~StandaloneSeCfg() {

}

string StandaloneSeCfg::get() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "se" << "\":\"" << se << "\",";
	ss << StandaloneCfg::get();
	ss << "}";

	return ss.str();
}

void StandaloneSeCfg::save() {

	StandaloneCfg::save(se);
}

void StandaloneSeCfg::del() {

}

} /* namespace common */
} /* namespace fts3 */
