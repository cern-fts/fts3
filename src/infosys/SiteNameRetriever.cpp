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
 * SiteNameRetriever.cpp
 *
 *  Created on: Feb 6, 2013
 *      Author: Michal Simon
 */

#include "SiteNameRetriever.h"

namespace fts3 {
namespace infosys {

SiteNameRetriever::SiteNameRetriever() : bdii(BdiiBrowser::getInstance()), myosg(OsgParser::getInstance()){


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
