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
namespace ws {

using namespace std;
using namespace fts3::common;

class StandaloneCfg : public Configuration {

public:

	StandaloneCfg() {}
	StandaloneCfg(CfgParser& parser);

	virtual ~StandaloneCfg();

	virtual string get();
	virtual void save() = 0;
	virtual void del() = 0;

protected:

	virtual void save(string name);

	void init(string name);

	/// active state
	bool active;

private:

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
