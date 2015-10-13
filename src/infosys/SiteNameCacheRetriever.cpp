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

#include "SiteNameCacheRetriever.h"

#include <iostream>
#include "common/Logger.h"

namespace fts3
{
namespace infosys
{

const char* SiteNameCacheRetriever::ATTR_GLUE1_SERVICE  = "GlueServiceUniqueID";
const char* SiteNameCacheRetriever::ATTR_GLUE1_LINK     = "GlueForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE1_SITE     = "GlueSiteUniqueID";
const char* SiteNameCacheRetriever::ATTR_GLUE1_ENDPOINT = "GlueServiceEndpoint";
const char* SiteNameCacheRetriever::ATTR_GLUE1_TYPE     = "GlueServiceType";
const char* SiteNameCacheRetriever::ATTR_GLUE1_VERSION  = "GlueServiceVersion";

const std::string SiteNameCacheRetriever::FIND_SE_SITE_GLUE1 =
    "("
    "   &"
    "   (GlueServiceUniqueID=*)"
    "   ("
    "       |"
    "       (GlueServiceType=SRM)"
    "       (GlueServiceType=xroot)"
    "       (GlueServiceType=webdav)"
    "       (GlueServiceType=gsiftp)"
    "       (GlueServiceType=http)"
    "       (GlueServiceType=https)"
    "   )"
    ")"
    ;
const char* SiteNameCacheRetriever::FIND_SE_SITE_ATTR_GLUE1[] =
{ATTR_GLUE1_SERVICE, ATTR_GLUE1_ENDPOINT, ATTR_GLUE1_LINK, ATTR_GLUE1_TYPE, ATTR_GLUE1_VERSION, 0};

const char* SiteNameCacheRetriever::ATTR_GLUE2_FK       = "GLUE2EndpointServiceForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE2_SITE     = "GLUE2ServiceAdminDomainForeignKey";
const char* SiteNameCacheRetriever::ATTR_GLUE2_ENDPOINT = "GLUE2EndpointURL";
const char* SiteNameCacheRetriever::ATTR_GLUE2_TYPE     = "GLUE2EndpointInterfaceName";
const char* SiteNameCacheRetriever::ATTR_GLUE2_VERSION  = "GLUE2EndpointInterfaceVersion";

const std::string SiteNameCacheRetriever::FIND_SE_FK_GLUE2 =
    "("
    "   &"
    "   (objectClass=GLUE2Endpoint)"
    "   (GLUE2EndpointURL=*)"
    "   ("
    "       |"
    "       (GLUE2EndpointInterfaceName=SRM)"
    "       (GLUE2EndpointInterfaceName=xroot)"
    "       (GLUE2EndpointInterfaceName=webdav)"
    "       (GLUE2EndpointInterfaceName=gsiftp)"
    "       (GLUE2EndpointInterfaceName=http)"
    "       (GLUE2EndpointInterfaceName=https)"
    "   )"
    ")"
    ;
const char* SiteNameCacheRetriever::FIND_SE_FK_ATTR_GLUE2[] =
{ATTR_GLUE2_ENDPOINT, ATTR_GLUE2_FK, ATTR_GLUE2_TYPE, ATTR_GLUE2_VERSION, 0};

const std::string SiteNameCacheRetriever::FIND_FK_SITE_GLUE2(std::string fk)
{
    std::stringstream ss;

    ss << "(";
    ss << " &";
    ss << " (objectClass=GLUE2Service)";
    ss << " (GLUE2ServiceID=" << fk << ")";
    ss << ")";
    return ss.str();
}
const char* SiteNameCacheRetriever::FIND_FK_SITE_ATTR_GLUE2[] = {ATTR_GLUE2_SITE, 0};


SiteNameCacheRetriever::~SiteNameCacheRetriever()
{

}

void SiteNameCacheRetriever::get(std::map<std::string, EndpointInfo>& cache)
{
    // get data from glue1
    fromGlue1(cache);
    // get data from glue2
    fromGlue2(cache);
}

void SiteNameCacheRetriever::fromGlue1(std::map<std::string, EndpointInfo>& cache)
{

    static BdiiBrowser& bdii = BdiiBrowser::instance();

    // browse for se names and respective site names
    time_t start = time(0);
    std::list< std::map<std::string, std::list<std::string> > > rs = bdii.browse< std::list<std::string> >(
                BdiiBrowser::GLUE1,
                FIND_SE_SITE_GLUE1,
                FIND_SE_SITE_ATTR_GLUE1
            );
    time_t stop = time(0);
    if (stop - start > 30)
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Querying BDII took more than 30s!" << commit;

    std::list< std::map<std::string, std::list<std::string> > >::iterator it;
    for (it = rs.begin(); it != rs.end(); ++it)
        {
            std::map<std::string, std::list<std::string> >& item = *it;

            // make sure this entry is not empty
            if (item[ATTR_GLUE1_ENDPOINT].empty() || item[ATTR_GLUE1_LINK].empty()) continue;
            // get the se name
            std::string se = item[ATTR_GLUE1_ENDPOINT].front();
            // get the corresponding site name
            std::string site = BdiiBrowser::parseForeingKey(item[ATTR_GLUE1_LINK], ATTR_GLUE1_SITE);
            // if the site name is not specified in the foreign key skip it
            if (site.empty()) continue;
            // cache the values
            cache[se].sitename = site;

            if (!item[ATTR_GLUE1_TYPE].empty())
                cache[se].type     = item[ATTR_GLUE1_TYPE].front();
            if (!item[ATTR_GLUE1_VERSION].empty())
                cache[se].version  = item[ATTR_GLUE1_VERSION].front();
        }
}

void SiteNameCacheRetriever::fromGlue2(std::map<std::string, EndpointInfo>& cache)
{

    static BdiiBrowser& bdii = BdiiBrowser::instance();

    // browse for se names and foreign keys that are pointing to the site name
    time_t start = time(0);
    std::list< std::map<std::string, std::list<std::string> > > rs = bdii.browse< std::list<std::string> >(
                BdiiBrowser::GLUE2,
                FIND_SE_FK_GLUE2,
                FIND_SE_FK_ATTR_GLUE2
            );
    time_t stop = time(0);
    // log a warning if browsing BDII takes more than 30s
    if (stop - start > 30)
        FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Querying BDII took more than 30s!" << commit;

    std::list< std::map<std::string, std::list<std::string> > >::iterator it;
    for (it = rs.begin(); it != rs.end(); ++it)
        {
            std::map<std::string, std::list<std::string> >& item = *it;
            // make sure this entry is not empty
            if (item[ATTR_GLUE2_ENDPOINT].empty() || item[ATTR_GLUE2_FK].empty()) continue;
            // get the se name
            std::string se = item[ATTR_GLUE2_ENDPOINT].front();
            // if it is already in the cache continue
            if (cache.find(se) == cache.end()) continue;
            // get the foreign key
            std::string fk = item[ATTR_GLUE2_FK].front();
            // browse for the site name
            start = time(0);
            std::list< std::map<std::string, std::list<std::string> > > rs2 = bdii.browse< std::list<std::string> >(
                        BdiiBrowser::GLUE2,
                        FIND_FK_SITE_GLUE2(fk),
                        FIND_FK_SITE_ATTR_GLUE2
                    );
            stop = time(0);
            // log a warning if browsing BDII takes more than 30s
            if (stop - start > 30)
                FTS3_COMMON_LOGGER_NEWLOG(WARNING) << "Querying BDII took more than 30s!" << commit;
            // make sure the result set is not empty
            if (rs2.empty() || rs2.front().empty() || rs2.front()[ATTR_GLUE2_SITE].empty()) continue;
            // get the respective site name
            std::string site = rs2.front()[ATTR_GLUE2_SITE].front();
            // cache the values
            cache[se].sitename = site;

            if (!item[ATTR_GLUE2_TYPE].empty())
                cache[se].type    = item[ATTR_GLUE2_TYPE].front();
            if (!item[ATTR_GLUE2_VERSION].empty())
                cache[se].version = item[ATTR_GLUE2_VERSION].front();
        }
}

} /* namespace infosys */
} /* namespace fts3 */
