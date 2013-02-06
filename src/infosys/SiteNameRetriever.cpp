/*
 * SiteNameRetriever.cpp
 *
 *  Created on: Feb 6, 2013
 *      Author: simonm
 */

#include "SiteNameRetriever.h"

namespace fts3 {
namespace infosys {

SiteNameRetriever::SiteNameRetriever() : bdii(BdiiBrowser::getInstance()){

}

SiteNameRetriever::~SiteNameRetriever() {

}

optional<string> SiteNameRetriever::getSiteName(string se) {
	// lock the cache
	mutex::scoped_lock lock(m);
	// check if the se is in cache
	map<string, string>::iterator it = seToSite.find(se);
	if (it != seToSite.end()) {
		return it->second;
	}
	// check in BDII
	string site = bdii.getSiteName(se);
	if (!site.empty()) {
		// save it in cache
		seToSite[se] = site;
		// clear the cache if there too many entries
		if(seToSite.size() > 5000) seToSite.clear();
		return site;
	}
	// check in MyOSG
	site = myosg.getName(se);
	if (!site.empty()) {
		// save in cache
		seToSite[se] = site;
		// clear the cache if there are too many entries
		if(seToSite.size() > 5000) seToSite.clear();
		return site;
	}
	// if nothing was found in BDII and MyOSD return an uninitialized optional
	return optional<string>();
}

} /* namespace infosys */
} /* namespace fts3 */
