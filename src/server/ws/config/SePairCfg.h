/*
 * SePairCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef SEPAIRCFG_H_
#define SEPAIRCFG_H_

#include "PairCfg.h"

namespace fts3 {
namespace ws {

class SePairCfg : public PairCfg {

public:

	SePairCfg(string dn, string source, string destination) : PairCfg(dn, source, destination) {};
	SePairCfg(string dn, CfgParser& parser);
	virtual ~SePairCfg();

	virtual string json();
	virtual void save();
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* SEPAIRCFG_H_ */
