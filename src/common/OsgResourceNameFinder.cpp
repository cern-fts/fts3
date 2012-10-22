/*
 * OsgResourceNameFinder.cpp
 *
 *  Created on: Oct 22, 2012
 *      Author: simonm
 */

#include "OsgResourceNameFinder.h"

using namespace fts3::common;

OsgResourceNameFinder::OsgResourceNameFinder(string path) {

	doc.load_file(path.c_str());
}

OsgResourceNameFinder::~OsgResourceNameFinder() {

}

string OsgResourceNameFinder::getName(string fqdn) {

	// the resource name
	string name;

	// look for the resource name (assume that the user has provided a fqdn)
    xpath_node node = doc.select_single_node(xpath_fqdn(fqdn).c_str());
    name = node.node().child_value("Name");

    if (!name.empty()) return name;

    // if the name was empty, check if the user provided an fqdn alias
    node = doc.select_single_node(xpath_fqdn_alias(fqdn).c_str());
    return node.node().child_value("Name");
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

