/*
 * VersionResolver.h
 *
 *  Created on: Dec 14, 2012
 *      Author: simonm
 */

#ifndef VERSIONRESOLVER_H_
#define VERSIONRESOLVER_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <string>

namespace fts3 {
namespace ws {

using namespace fts3::common;
using namespace std;

class VersionResolver  : public ThreadSafeInstanceHolder<VersionResolver> {

public:
	VersionResolver();
	virtual ~VersionResolver();

	string getVersion();
	string getInterface();
	string getSchema();
	string getMetadata();

private:

	string version;
	string interface;
	string schema;
	string metadata;
};

} /* namespace server */
} /* namespace fts3 */
#endif /* VERSIONRESOLVER_H_ */
