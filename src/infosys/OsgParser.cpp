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
 * OsgParser.cpp
 *
 *  Created on: Jan 14, 2013
 *      Author: Michał Simon
 */

#include "OsgParser.h"

#include "config/serverconfig.h"

namespace fts3 { namespace infosys {

using namespace config;


const string OsgParser::NAME_PROPERTY = "Name";
const string OsgParser::ACTIVE_PROPERTY = "Active";
const string OsgParser::DISABLE_PROPERTY = "Disable";

const string OsgParser::STR_TRUE = "True";


const string OsgParser::myosg_path = "/var/lib/fts3/osg.xml";

OsgParser::OsgParser(string path) {

	doc.load_file(path.c_str());
}

OsgParser::~OsgParser() {

}

bool OsgParser::isInUse() {

	static const string myosg_str = "myosg";

	vector<string> providers = theServerConfig().get< vector<string> >("InfoProviders");
	vector<string>::iterator it;

	for (it = providers.begin(); it != providers.end(); it++) {
		if (myosg_str == *it) return true;
	}

	return false;
}

string OsgParser::get(string fqdn, string property) {

	// if not on the list containing info providers return an empty string
	if (!isInUse()) return string();

	// if the MyOSG was set to 'flase' return an empty string
	if (!theServerConfig().get<bool>("MyOSG")) return string();

	// look for the resource name (assume that the user has provided a fqdn)
    xpath_node node = doc.select_single_node(xpath_fqdn(fqdn).c_str());
    string val = node.node().child_value(property.c_str());

    if (!val.empty()) return val;

    // if the name was empty, check if the user provided an fqdn alias
    node = doc.select_single_node(xpath_fqdn_alias(fqdn).c_str());
    return node.node().child_value(property.c_str());
}

string OsgParser::getSiteName(string fqdn) {

	return get(fqdn, NAME_PROPERTY);
}

optional<bool> OsgParser::isActive(string fqdn) {
	string val = get(fqdn, ACTIVE_PROPERTY);
	if (val.empty()) return optional<bool>();
	return val == STR_TRUE;
}

optional<bool> OsgParser::isDisabled(string fqdn) {
	string val = get(fqdn, DISABLE_PROPERTY);
	if (val.empty()) return optional<bool>();
	return val == STR_TRUE;
}

string OsgParser::xpath_fqdn(string fqdn) {
	static const string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDN='";
	static const string xpath_end = "']";

	return xpath_fqdn + fqdn + xpath_end;
}

string OsgParser::xpath_fqdn_alias(string alias) {
	static const string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDNAliases/FQDNAlias='";
	static const string xpath_end = "']";

	return xpath_fqdn + alias + xpath_end;
}

} /* namespace cli */
} /* namespace fts3 */
