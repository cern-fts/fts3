/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use this file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 *
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
