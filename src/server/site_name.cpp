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
 *	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *	See the License for the specific language governing permissions and
 *	limitations under the License.
 *
 */

#include "site_name.h"
#include "parse_url.h"
#include "logger.h"
#include "error.h"

#include "infosys/SiteNameRetriever.h"

using namespace fts3::common;
using namespace fts3::infosys;

SiteName::SiteName()
{

}

SiteName::~SiteName()
{
}


std::string SiteName::getSiteName(std::string& hostname)
{

    std::string se;
    char *base_scheme = NULL;
    char *base_host = NULL;
    char *base_path = NULL;
    int base_port = 0;
    parse_url(hostname.c_str(), &base_scheme, &base_host, &base_port, &base_path);
    if(base_host) se = std::string(base_host);

    if(base_scheme) free(base_scheme);
    if(base_host)   free(base_host);
    if(base_path)   free(base_path);

    std::string site =  SiteNameRetriever::getInstance().getSiteName(se);
    if(site.length() == 0)
        return std::string("");
    else if(site.length() > 0 && site != "null")
        return site;
    else
        return std::string("");

}


