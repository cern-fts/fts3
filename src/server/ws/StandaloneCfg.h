/*
 * StandaloneSeCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef STANDALONECFG_H_
#define STANDALONECFG_H_

#include "Configuration.h"

#include <string>
#include <map>

namespace fts3 {
namespace common {

using namespace std;

struct StandaloneCfg : Configuration {

	StandaloneCfg(CfgParser& parser);

	virtual ~StandaloneCfg();

	virtual string json () = 0;

	/// active state
	bool active;

	/// inbound share
	map<string, int> in_share;
	/// inbound protocol
	map<string, int> in_protocol;

	/// outbound share
	map<string, int> out_share;
	/// outbound protocol
	map<string, int> out_protocol;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONESECFG_H_ */
