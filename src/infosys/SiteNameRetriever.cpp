/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SiteNameRetriever.h"

#include "../config/ServerConfig.h"
#include "common/Logger.h"


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

const std::string SiteNameRetriever::FIND_SE_SITE_GLUE2(std::string se)
{
    std::stringstream ss;
    ss << "(&";
    ss << "(" << BdiiBrowser::ATTR_OC << "=" << BdiiBrowser::CLASS_SERVICE_GLUE2 << ")";
    ss << "(" << ATTR_GLUE2_SERVICE << "=*" << se << "*)";
    ss << ")";

    return ss.str();
}
const char* SiteNameRetriever::FIND_SE_SITE_ATTR_GLUE2[] = {ATTR_GLUE2_SITE, 0};

const std::string SiteNameRetriever::FIND_SE_SITE_GLUE1(std::string se)
{
    std::stringstream ss;
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

std::string SiteNameRetriever::getFromBdii(std::string se)
{

    BdiiBrowser& bdii = BdiiBrowser::instance();

    // first check glue2
    std::list< std::map<std::string, std::list<std::string> > > rs = bdii.browse< std::list<std::string> >(
                BdiiBrowser::GLUE2,
                FIND_SE_SITE_GLUE2(se),
                FIND_SE_SITE_ATTR_GLUE2
            );

    if (!rs.empty())
        {
            if (!rs.front()[ATTR_GLUE2_SITE].empty())
                {
                    std::string str =  rs.front()[ATTR_GLUE2_SITE].front();
                    return str;
                }
        }

    // then check glue1
    rs = bdii.browse< std::list<std::string> >(
             BdiiBrowser::GLUE1,
             FIND_SE_SITE_GLUE1(se),
             FIND_SE_SITE_ATTR_GLUE1
         );

    if (rs.empty())
        {
            if (bdii.checkIfInUse(BdiiBrowser::GLUE2) || bdii.checkIfInUse(BdiiBrowser::GLUE1))
                {
                    FTS3_COMMON_LOGGER_NEWLOG (WARNING) << "SE: " << se << " has not been found in the BDII" << commit;
                }
            return std::string();
        }

    std::list<std::string> values = rs.front()[ATTR_GLUE1_LINK];
    std::string site = BdiiBrowser::parseForeingKey(values, ATTR_GLUE1_SITE);

    if (site.empty() && !rs.front()[ATTR_GLUE1_HOSTINGORG].empty())
        {
            site = rs.front()[ATTR_GLUE1_HOSTINGORG].front();
        }

    return site;
}

std::string SiteNameRetriever::getSiteName(std::string se)
{
    // check if the infosys has been activated in the fts3config file
    bool active = config::ServerConfig::instance().get<bool>("Infosys");
    if (!active) return std::string();

    // lock the cache
    boost::mutex::scoped_lock lock(m);

    // check if the se is in cache
    std::map<std::string, std::string>::iterator it = seToSite.find(se);
    if (it != seToSite.end())
        {
            return it->second;
        }

    std::string site;

#ifndef WITHOUT_PUGI
    // check in BDII cache
    site = BdiiCacheParser::instance().getSiteName(se);
    if (!site.empty())
        {
            // save it in cache
            seToSite[se] = site;
            // clear the cache if there too many entries
            if(seToSite.size() > 5000) seToSite.clear();
            return site;
        }
#endif

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

#ifndef WITHOUT_PUGI
    // check in MyOSG
    site = OsgParser::instance().getSiteName(se);

    // update the cache
    if (!site.empty())
        {
            // save in cache
            seToSite[se] = site;
            // clear the cache if there are too many entries
            if(seToSite.size() > 5000) seToSite.clear();
        }
#endif

    return site;
}

} /* namespace infosys */
} /* namespace fts3 */
