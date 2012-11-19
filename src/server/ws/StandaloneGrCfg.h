/*
 * StandaloneGrCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef STANDALONEGRCFG_H_
#define STANDALONEGRCFG_H_

#include "StandaloneCfg.h"

#include <string>
#include <vector>

namespace fts3 {
namespace common {

using namespace std;

struct StandaloneGrCfg : StandaloneCfg {

	StandaloneGrCfg(CfgParser& parser);
	virtual ~StandaloneGrCfg();

	string json() {
		return "";
	}

	string group;
	vector<string> members;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONEGRCFG_H_ */
