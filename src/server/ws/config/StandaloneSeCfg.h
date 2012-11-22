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
namespace ws {

using namespace std;
using namespace fts3::common;

class StandaloneSeCfg : public StandaloneCfg {

public:

	StandaloneSeCfg(string dn, string name);
	StandaloneSeCfg(string dn, CfgParser& parser);
	virtual ~StandaloneSeCfg();

	virtual string json();
	virtual void save();
	virtual void del();

private:

	string se;
};

} /* namespace common */
} /* namespace fts3 */
#endif /* STANDALONESECFG_H_ */
