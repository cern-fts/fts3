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
namespace ws {

using namespace std;
using namespace fts3::common;

class StandaloneGrCfg : public StandaloneCfg {

public:

	StandaloneGrCfg(string name);
	StandaloneGrCfg(CfgParser& parser);
	virtual ~StandaloneGrCfg();

	virtual string json();
	virtual void save();
	virtual void del();

private:

	string group;
	vector<string> members;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONEGRCFG_H_ */
