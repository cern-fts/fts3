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

StandaloneSeCfg::StandaloneSeCfg(string dn, string name) : StandaloneCfg(dn), se(name) {

	// get SE active state
	Se* se = 0;
	db->getSe(se, name);
	if (se) {
		active = se->STATE == on;
		delete se;
	} else
		throw Err_Custom("The SE: " + name + " does not exist!");

	init(name);
}

StandaloneSeCfg::StandaloneSeCfg(string dn, CfgParser& parser) : StandaloneCfg(dn, parser)  {

	se = parser.get<string>("se");
	all = json();
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
	addSe(se, active);
	StandaloneCfg::save(se);
}

void StandaloneSeCfg::del() {
	eraseSe(se);
	StandaloneCfg::del(se);
}

} /* namespace common */
} /* namespace fts3 */
