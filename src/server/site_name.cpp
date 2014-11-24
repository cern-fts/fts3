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
    Uri uri = Uri::Parse(hostname);

    if(uri.Host.length()) {
        std::string site =  SiteNameRetriever::getInstance().getSiteName(uri.Host);
        if(site.length() > 0 && site != "null")
            return site;
        else
            return std::string();
    }
    else {
        return std::string("");
    }
}
