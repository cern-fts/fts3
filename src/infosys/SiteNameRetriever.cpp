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

namespace fts3
{
namespace infosys
{

const char* SiteNameRetriever::ATTR_GLUE1_SERVICE = "GlueServiceUniqueID";
const char* SiteNameRetriever::ATTR_GLUE1_SERVICE_URI = "GlueServiceURI";


const char* SiteNameRetriever::ATTR_GLUE1_LINK = "GlueForeignKey";
const char* SiteNameRetriever::ATTR_GLUE1_SITE = "GlueSiteUniqueID";
const char* SiteNameRetriever::ATTR_GLUE1_HOSTINGORG = "GlueServiceHostingOrganization";

const char* SiteNameRetriever::ATTR_GLUE2_SERVICE = "GLUE2ServiceID";
const char* SiteNameRetriever::ATTR_GLUE2_SITE = "GLUE2ServiceAdminDomainForeignKey";

const string SiteNameRetriever::FIND_SE_SITE_GLUE2(string se)
{
    stringstream ss;
    ss << "(&";
    ss << "(" << BdiiBrowser::ATTR_OC << "=" << BdiiBrowser::CLASS_SERVICE_GLUE2 << ")";
    ss << "(" << ATTR_GLUE2_SERVICE << "=*" << se << "*)";
    ss << ")";

    return ss.str();
}
const char* SiteNameRetriever::FIND_SE_SITE_ATTR_GLUE2[] = {ATTR_GLUE2_SITE, 0};

const string SiteNameRetriever::FIND_SE_SITE_GLUE1(string se)
{
    stringstream ss;
    ss << "(&";
    ss << "(" << BdiiBrowser::ATTR_OC << "=" << BdiiBrowser::CLASS_SERVICE_GLUE1 << ")";
    ss << "(|(" << ATTR_GLUE1_SERVICE << "=*" << se << "*)";
    ss << "(" << ATTR_GLUE1_SERVICE_URI << "=*" << se << "*))";
    ss << ")";
    return ss.str();
}
const char* SiteNameRetriever::FIND_SE_SITE_ATTR_GLUE1[] = {ATTR_GLUE1_LINK, ATTR_GLUE1_HOSTINGORG, 0};

SiteNameRetriever::~SiteNameRetriever()
{

}

string SiteNameRetriever::getFromBdii(string se)
{

    BdiiBrowser& bdii = BdiiBrowser::getInstance();

    // first check glue2
    list< map<string, list<string> > > rs = bdii.browse< list<string> >(
            BdiiBrowser::GLUE2,
            FIND_SE_SITE_GLUE2(se),
            FIND_SE_SITE_ATTR_GLUE2
		);

    if (!rs.empty())
        {
            if (!rs.front()[ATTR_GLUE2_SITE].empty())
                {
                    string str =  rs.front()[ATTR_GLUE2_SITE].front();
                    return str;
                }
        }

    // then check glue1
    rs = bdii.browse< list<string> >(
             BdiiBrowser::GLUE1,
             FIND_SE_SITE_GLUE1(se),
             FIND_SE_SITE_ATTR_GLUE1
         );

    if (rs.empty()) return string();

    list<string> values = rs.front()[ATTR_GLUE1_LINK];
    string site = BdiiBrowser::parseForeingKey(values, ATTR_GLUE1_SITE);

    if (site.empty() && !rs.front()[ATTR_GLUE1_HOSTINGORG].empty())
        {
            site = rs.front()[ATTR_GLUE1_HOSTINGORG].front();
        }

    return site;
}

string SiteNameRetriever::getSiteName(string se)
{
    // lock the cache
    mutex::scoped_lock lock(m);

    // check if the se is in cache
    map<string, string>::iterator it = seToSite.find(se);
    if (it != seToSite.end())
        {
            return it->second;
        }

    // check in BDII cache
    string site = BdiiCacheParser::getInstance().getSiteName(se);
    if (!site.empty())
        {
            // save it in cache
            seToSite[se] = site;
            // clear the cache if there too many entries
            if(seToSite.size() > 5000) seToSite.clear();
            return site;
        }

    // check in BDII
    site = getFromBdii(se);
    if (!site.empty())
        {
            // save it in cache
            seToSite[se] = site;
            // clear the cache if there too many entries
            if(seToSite.size() > 5000) seToSite.clear();
            return site;
        }

    // check in MyOSG
    site = OsgParser::getInstance().getSiteName(se);

    // update the cache
    if (!site.empty())
        {
            // save in cache
            seToSite[se] = site;
            // clear the cache if there are too many entries
            if(seToSite.size() > 5000) seToSite.clear();
        }

    return site;
}

} /* namespace infosys */
} /* namespace fts3 */
