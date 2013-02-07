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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or impltnsied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 * OsgParser.h
 *
 *  Created on: Jan 14, 2013
 *      Author: Michał Simon
 */

#ifndef OSGPARSER_H_
#define OSGPARSER_H_

#include "common/ThreadSafeInstanceHolder.h"

#include <pugixml.hpp>
#include <string>
#include <boost/optional.hpp>

namespace fts3 { namespace infosys {

using namespace std;
using namespace boost;
using namespace pugi;

using namespace fts3::common;

class OsgParser : public ThreadSafeInstanceHolder<OsgParser> {

	friend class ThreadSafeInstanceHolder<OsgParser>;

public:

	virtual ~OsgParser();

	string getSiteName(string fqdn);
	optional<bool> isActive(string fqdn);
	optional<bool> isDisabled(string fqdn);

private:

	OsgParser(string path = myosg_path);
	OsgParser(OsgParser const&);
	OsgParser& operator=(OsgParser const&);

	string get(string fqdn, string property);

	static const string NAME_PROPERTY;
	static const string ACTIVE_PROPERTY;
	static const string DISABLE_PROPERTY;

	static const string STR_TRUE;

	xml_document doc;

	static string xpath_fqdn(string fqdn);
	static string xpath_fqdn_alias(string alias);

	/// default path to MyOSG file
	static const string myosg_path;
};

} /* namespace cli */
} /* namespace fts3 */
#endif /* OSGPARSER_H_ */
