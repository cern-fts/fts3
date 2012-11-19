/*
 * Configuration.h
 *
 *  Created on: Nov 19, 2012
 *      Author: simonm
 */

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "CfgParser.h"

#include <string>

namespace fts3 {
namespace common {

using namespace std;

struct Configuration {

	Configuration(CfgParser& parser) {};
	virtual ~Configuration() {};

	virtual string json() = 0;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* CONFIGURATION_H_ */
