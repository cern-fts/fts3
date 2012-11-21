/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneSeCfg.h"

#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

namespace fts3 {
namespace ws {

using namespace boost;

StandaloneSeCfg::StandaloneSeCfg(string name) : se(name) {

	// get SE active state
	Se* se = 0;
	db->getSe(se, name);
	if (se) {
		active = se->STATE == "on";
		delete se;
	} else
		throw Err_Custom("The SE: " + name + " does not exist!");

	init(name);
}

StandaloneSeCfg::StandaloneSeCfg(CfgParser& parser) : StandaloneCfg(parser)  {
	se = parser.get<string>("se");
	addSe(se);
}

StandaloneSeCfg::~StandaloneSeCfg() {

}

string StandaloneSeCfg::json() {

	stringstream ss;

	ss << "{";
	ss << "\"" << "se" << "\":\"" << se << "\",";
	ss << StandaloneCfg::json();
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
