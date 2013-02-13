/*
 * SiteNameCacheRetriver.cpp
 *
 *  Created on: Feb 13, 2013
 *      Author: simonm
 */

#include "SiteNameCacheRetriever.h"

namespace fts3 {
namespace infosys {

const char* SiteNameCacheRetriever::ATTR_GLUE1_SERVICE = "GlueServiceUniqueID";
const char* SiteNameCacheRetriever::ATTR_GLUE1_LINK = "GlueForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE1_SITE = "GlueSiteUniqueID";

const string SiteNameCacheRetriever::FIND_SE_SITE_GLUE1 = "(|(GlueServiceUniqueID=*))";
const char* SiteNameCacheRetriever::FIND_SE_SITE_ATTR_GLUE1[] = {ATTR_GLUE1_SERVICE, ATTR_GLUE1_LINK, 0};


SiteNameCacheRetriever::~SiteNameCacheRetriever() {

}

ptree SiteNameCacheRetriever::get() {
	// get data from glue1
	fromGlue1();
	// get data from glue2
	fromGlue2();
	// put the data into ptree

	ptree root;

	map<string, string>::iterator it;
	for (it = cache.begin(); it != cache.end(); it++) {
		ptree entry;
		entry.add("hostname", it->first);
		entry.add("sitename", it->second);
		root.add_child("entry", entry);
	}

	return root;
}

void SiteNameCacheRetriever::fromGlue1() {

	BdiiBrowser& bdii = BdiiBrowser::getInstance();

	list< map<string, list<string> > > rs = bdii.browse< list<string> >(
			BdiiBrowser::GLUE1,
			FIND_SE_SITE_GLUE1,
			FIND_SE_SITE_ATTR_GLUE1
		);

	if (rs.empty()) return;

	list< map<string, list<string> > >::iterator it;
	for (it = rs.begin(); it != rs.end(); it++) {
		map<string, list<string> >& item = *it;

		if (item[ATTR_GLUE1_SERVICE].empty() || item[ATTR_GLUE1_LINK].empty()) continue;

		// get the se name
		string se = item[ATTR_GLUE1_SERVICE].front();
		// get the corresponding site name
		string site = BdiiBrowser::parseForeingKey(item[ATTR_GLUE1_LINK], ATTR_GLUE1_SITE);
		// cache the values
		cache[se] = site;
	}
}

void SiteNameCacheRetriever::fromGlue2() {

}

} /* namespace infosys */
} /* namespace fts3 */
