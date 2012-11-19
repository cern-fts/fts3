/*
 * StandaloneSeCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef STANDALONESECFG_H_
#define STANDALONESECFG_H_

#include "StandaloneCfg.h"

#include <string>

namespace fts3 {
namespace common {

using namespace std;

struct StandaloneSeCfg : StandaloneCfg {
public:
	StandaloneSeCfg(CfgParser& parser);
	virtual ~StandaloneSeCfg();

	string json() {
		return "";
	}

	string se;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONESECFG_H_ */
