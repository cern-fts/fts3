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
 * InternalLogRetriever.h
 *
 *  Created on: Oct 18, 2012
 *      Author: simonm
 */

#ifndef INTERNALLOGRETRIEVER_H_
#define INTERNALLOGRETRIEVER_H_

#include "ws-ifce/gsoap/gsoap_stubs.h"

#include <string>
#include <list>

namespace fts3 { namespace ws {

using namespace std;

class InternalLogRetriever {

public:
	InternalLogRetriever(string endpoint);
	virtual ~InternalLogRetriever();

	list<string> getInternalLogs(string jobId);

private:
	///
	string endpoint;

	///
	soap* ctx;
};

}
}

#endif /* INTERNALLOGRETRIEVER_H_ */
