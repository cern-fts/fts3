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

	GrPairCfg(string source, string destination) : PairCfg(source, destination) {}; // check if SE groups exist
	GrPairCfg(CfgParser& parser);
	virtual ~GrPairCfg();

	virtual string get();
	virtual void del();

private:

	void checkGroup(string group);
};

} /* namespace ws */
} /* namespace fts3 */
#endif /* GRPAIRCFG_H_ */
