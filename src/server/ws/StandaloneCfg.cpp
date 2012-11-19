/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneCfg.h"

namespace fts3 {
namespace common {

StandaloneCfg::StandaloneCfg(CfgParser& parser) : Configuration(parser) {

	active = parser.get<bool>("active");
	in_share = parser.get< map<string, int> >("in.share");
	in_protocol = parser.get< map<string, int> >("in.protocol");
	out_share = parser.get< map<string, int> >("out.share");
	out_protocol = parser.get< map<string, int> >("out.protocol");
}

StandaloneCfg::~StandaloneCfg() {

}

} /* namespace common */
} /* namespace fts3 */
