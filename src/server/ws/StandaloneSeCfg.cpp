/*
 * StandaloneSeCfg.cpp
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#include "StandaloneSeCfg.h"

namespace fts3 {
namespace common {

StandaloneSeCfg::StandaloneSeCfg(CfgParser& parser) : StandaloneCfg(parser)  {
	se = parser.get<string>("se");
}

StandaloneSeCfg::~StandaloneSeCfg() {

}

} /* namespace common */
} /* namespace fts3 */
