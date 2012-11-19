/*
 * StandaloneGrCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneGrCfg.h"

namespace fts3 {
namespace common {

StandaloneGrCfg::StandaloneGrCfg(CfgParser& parser) : StandaloneCfg(parser) {
	group = parser.get<string>("group");
	members = parser.get< vector<string> >("members");
}

StandaloneGrCfg::~StandaloneGrCfg() {
}

} /* namespace common */
} /* namespace fts3 */
