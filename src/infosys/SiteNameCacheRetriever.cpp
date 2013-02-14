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
 * SiteNameCacheRetriver.cpp
 *
 *  Created on: Feb 13, 2013
 *      Author: Michał Simon
 */

#include "SiteNameCacheRetriever.h"
#include <iostream>

namespace fts3 {
namespace infosys {

const char* SiteNameCacheRetriever::ATTR_GLUE1_SERVICE = "GlueServiceUniqueID";
const char* SiteNameCacheRetriever::ATTR_GLUE1_LINK = "GlueForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE1_SITE = "GlueSiteUniqueID";

const string SiteNameCacheRetriever::FIND_SE_SITE_GLUE1 =
		"("
		"	&"
		"	(GlueServiceUniqueID=*)"
		"	("
		"		|"
		"		(GlueServiceType=SRM)"
		"		(GlueServiceType=xroot)"
		"		(GlueServiceType=webdav)"
		"		(GlueServiceType=gsiftp)"
		"		(GlueServiceType=http)"
		"		(GlueServiceType=https)"
		"	)"
		")"
		;
const char* SiteNameCacheRetriever::FIND_SE_SITE_ATTR_GLUE1[] = {ATTR_GLUE1_SERVICE, ATTR_GLUE1_LINK, 0};

const char* SiteNameCacheRetriever::ATTR_GLUE2_FK = "GLUE2EndpointServiceForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE2_SITE = "GLUE2ServiceAdminDomainForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE2_ENDPOINT = "GLUE2EndpointURL";

const string SiteNameCacheRetriever::FIND_SE_FK_GLUE2 =
		"("
		"	&"
		"	(objectClass=GLUE2Endpoint)"
		"	(GLUE2EndpointURL=*)"
		"	("
		"		|"
		"		(GLUE2EndpointInterfaceName=SRM)"
		"		(GLUE2EndpointInterfaceName=xroot)"
		"		(GLUE2EndpointInterfaceName=webdav)"
		"		(GLUE2EndpointInterfaceName=gsiftp)"
		"		(GLUE2EndpointInterfaceName=http)"
		"		(GLUE2EndpointInterfaceName=https)"
		"	)"
		")"
		;
const char* SiteNameCacheRetriever::FIND_SE_FK_ATTR_GLUE2[] = {ATTR_GLUE2_ENDPOINT, ATTR_GLUE2_FK, 0};

const string SiteNameCacheRetriever::FIND_FK_SITE_GLUE2(string fk) {
	stringstream ss;
	ss << "(";
	ss << "	&";
	ss << "	(objectClass=GLUE2Service)";
	ss << "	(GLUE2ServiceID=" << fk << ")";
	ss << ")";
	return ss.str();
}
const char* SiteNameCacheRetriever::FIND_FK_SITE_ATTR_GLUE2[] = {ATTR_GLUE2_SITE, 0};


SiteNameCacheRetriever::~SiteNameCacheRetriever() {

}

void SiteNameCacheRetriever::get(ptree& root) {
	// get data from glue1
	fromGlue1();
	// get data from glue2
	fromGlue2();
	// put the data into ptree:
	// iterate over the results
	map<string, string>::iterator it;
	for (it = cache.begin(); it != cache.end(); it++) {
		// a single entry
		ptree entry;
		// contains the hostname
		entry.add("hostname", it->first);
		// and the sitename
		entry.add("sitename", it->second);
		// add it to the root
		root.add_child("entry", entry);
	}
}

void SiteNameCacheRetriever::fromGlue1() {

	static BdiiBrowser& bdii = BdiiBrowser::getInstance();

	// browse for se names and respective site names
	list< map<string, list<string> > > rs = bdii.browse< list<string> >(
			BdiiBrowser::GLUE1,
			FIND_SE_SITE_GLUE1,
			FIND_SE_SITE_ATTR_GLUE1
		);

	list< map<string, list<string> > >::iterator it;
	for (it = rs.begin(); it != rs.end(); it++) {
		map<string, list<string> >& item = *it;

		// make sure this entry is not empty
		if (item[ATTR_GLUE1_SERVICE].empty() || item[ATTR_GLUE1_LINK].empty()) continue;
		// get the se name
		string se = item[ATTR_GLUE1_SERVICE].front();
		// get the corresponding site name
		string site = BdiiBrowser::parseForeingKey(item[ATTR_GLUE1_LINK], ATTR_GLUE1_SITE);
		// if the site name is not specified in the foreign key skip it
		if (site.empty()) continue;
		// cache the values
		cache[se] = site;
	}
}

void SiteNameCacheRetriever::fromGlue2() {

	static BdiiBrowser& bdii = BdiiBrowser::getInstance();

	// browse for se names and foreign keys that are pointing to the site name
	list< map<string, list<string> > > rs = bdii.browse< list<string> >(
			BdiiBrowser::GLUE2,
			FIND_SE_FK_GLUE2,
			FIND_SE_FK_ATTR_GLUE2
		);

	list< map<string, list<string> > >::iterator it;
	for (it = rs.begin(); it != rs.end(); it++) {
		map<string, list<string> >& item = *it;
		// make sure this entry is not empty
		if (item[ATTR_GLUE2_ENDPOINT].empty() || item[ATTR_GLUE2_FK].empty()) continue;
		// get the se name
		string se = item[ATTR_GLUE2_ENDPOINT].front();
		// if it is already in the cache continue
		if (cache.find(se) == cache.end()) continue;
		// get the foreign key
		string fk = item[ATTR_GLUE2_FK].front();
		// browse for the site name
		list< map<string, list<string> > > rs2 = bdii.browse< list<string> >(
				BdiiBrowser::GLUE2,
				FIND_FK_SITE_GLUE2(fk),
				FIND_FK_SITE_ATTR_GLUE2
			);
		// make sure the result set is not empty
		if (rs2.empty() || rs2.front().empty() || rs2.front()[ATTR_GLUE2_SITE].empty()) continue;
		// get the respective site name
		string site = rs2.front()[ATTR_GLUE2_SITE].front();
		// cache the values
		cache[se] = site;
	}
}

} /* namespace infosys */
} /* namespace fts3 */
