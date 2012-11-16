/*
 * OsgResourceNameFinder.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: simonm
 */

#include "OsgResourceNameFinder.h"

using namespace fts3::common;

const string OsgResourceNameFinder::NAME_PROPERTY = "Name";
const string OsgResourceNameFinder::ACTIVE_PROPERTY = "Active";
const string OsgResourceNameFinder::DISABLE_PROPERTY = "Disable";

const string OsgResourceNameFinder::TRUE = "True";

OsgResourceNameFinder::OsgResourceNameFinder(string path) {

	doc.load_file(path.c_str());
}

OsgResourceNameFinder::~OsgResourceNameFinder() {

}

string OsgResourceNameFinder::get(string fqdn, string property) {

	// look for the resource name (assume that the user has provided a fqdn)
    xpath_node node = doc.select_single_node(xpath_fqdn(fqdn).c_str());
    string val = node.node().child_value(property.c_str());

    if (!val.empty()) return val;

    // if the name was empty, check if the user provided an fqdn alias
    node = doc.select_single_node(xpath_fqdn_alias(fqdn).c_str());
    return node.node().child_value(property.c_str());
}

string OsgResourceNameFinder::getName(string fqdn) {

	return get(fqdn, NAME_PROPERTY);
}

bool OsgResourceNameFinder::isActive(string fqdn) {
	return get(fqdn, ACTIVE_PROPERTY) == TRUE;
}

bool OsgResourceNameFinder::isDisabled(string fqdn) {
	return get(fqdn, DISABLE_PROPERTY) == TRUE;
}

string OsgResourceNameFinder::xpath_fqdn(string fqdn) {
	static const string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDN='";
	static const string xpath_end = "']";

	return xpath_fqdn + fqdn + xpath_end;
}

string OsgResourceNameFinder::xpath_fqdn_alias(string alias) {
	static const string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDNAliases/FQDNAlias='";
	static const string xpath_end = "']";

	return xpath_fqdn + alias + xpath_end;
}

