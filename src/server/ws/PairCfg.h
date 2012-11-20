/*
 * PairCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef PAIRCFG_H_
#define PAIRCFG_H_

#include "Configuration.h"

#include <string>
#include <map>

namespace fts3 {
namespace ws {

using namespace std;
using namespace fts3::common;

class PairCfg : public Configuration {

public:

	PairCfg(CfgParser& parser);
	virtual ~PairCfg();

	virtual string get();
	virtual void save();
	virtual void del() = 0;

protected:
	/// source
	string source;
	/// destination
	string destination;

private:
	/// symbolic name
	string symbolic_name;
	/// active state
	bool active;
	/// inbound share
	map<string, int> share;
	/// inbound protocol
	map<string, int> protocol;
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* PAIRCFG_H_ */
