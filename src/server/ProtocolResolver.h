/*
 *	Copyright notice:
 *	Copyright © Members of the EMI Collaboration, 2010.
 *
 *	See www.eu-emi.eu for details on the copyright holders
 *
 *	Licensed under the Apache License, Version 2.0 (the "License");
 *	you may not use soap file except in compliance with the License.
 *	You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 *	Unless required by applicable law or agreed to in writing, software
 *	distributed under the License is distributed on an "AS IS" BASIS,
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implcfgied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * ProtocolResolver.h
 *
 *  Created on: Dec 3, 2012
 *      Author: Michal Simon
 */

#ifndef PROTOCOLRESOLVER_H_
#define PROTOCOLRESOLVER_H_

#include "server_dev.h"

#include "db/generic/SingleDbInstance.h"

#include <string>
#include <utility>

#include <boost/optional.hpp>

FTS3_SERVER_NAMESPACE_START

using namespace std;
using namespace db;
using namespace boost;

class ProtocolResolver {

	enum LinkType {
		SE_PAIR = 0,
		SOURCE_SE,
		DESTINATION_SE,
		GR_PAIR,
		SOURCE_GR,
		DESTINATION_GR
	};

	enum {
		SOURCE = 0,
		DESTINATION,
		VO
	};

public:
	ProtocolResolver(string job_id);
	virtual ~ProtocolResolver();
	SeProtocolConfig* resolve();

private:

	bool isGr(string name);

	SeProtocolConfig* getProtocolCfg(optional< pair<string, string> > link);

	SeProtocolConfig* merge(SeProtocolConfig* source_ptr, SeProtocolConfig* destination_ptr);



	///
	string job_id;
	/// DB singleton instance
	GenericDbIfce* db;

	optional< pair<string, string> > link[8];

};

FTS3_SERVER_NAMESPACE_END

#endif /* PROTOCOLRESOLVER_H_ */
