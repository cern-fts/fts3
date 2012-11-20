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

//StandaloneSeCfg::StandaloneSeCfg(string name) {
//
//	// get SE active state
//	Se* se = 0;
//	db->getSe(se, name);
//	if (se) {
//		active = se->STATE == "on";
//		delete se;
//	} else
//		throw Err_Custom("The SE: " + name + " does not exist!");
//
//	// get SE in and out share
//	in_share = getShare("*", name);
//	out_share = getShare(name, "*");
//
//	// get SE in and out protocol
//	in_protocol = getProtocol(0, "*-" + name);
//	out_protocol = getProtocol(0, name + "-*");
//}

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
