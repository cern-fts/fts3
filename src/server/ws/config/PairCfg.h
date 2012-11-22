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

#include <boost/optional.hpp>

namespace fts3 {
namespace ws {

using namespace std;
using namespace boost;
using namespace fts3::common;

class PairCfg : public Configuration {

public:

	PairCfg(string dn, string source, string destination);
	PairCfg(string dn, CfgParser& parser);
	virtual ~PairCfg();

	virtual string json();
	virtual void save();
	virtual void del();

protected:
	/// source
	string source;
	/// destination
	string destination;
	/// optional symbolic name
	optional<string> symbolic_name_opt;
	/// symbolic name (given by user or generated)
	string symbolic_name;

private:
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
