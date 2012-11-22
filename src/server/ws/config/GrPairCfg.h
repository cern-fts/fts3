/*
 * GrPairCfg.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef GRPAIRCFG_H_
#define GRPAIRCFG_H_

#include "PairCfg.h"

namespace fts3 {
namespace ws {

class GrPairCfg : public PairCfg {

public:

	GrPairCfg(string dn, string source, string destination) : PairCfg(dn, source, destination) {}; // check if SE groups exist
	GrPairCfg(string dn, CfgParser& parser);
	virtual ~GrPairCfg();

	virtual string json();
	virtual void save();
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* GRPAIRCFG_H_ */
