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

#include "OsgParser.h"

#include "../config/ServerConfig.h"

namespace fts3
{
namespace infosys
{

using namespace config;


const std::string OsgParser::NAME_PROPERTY = "Name";
const std::string OsgParser::ACTIVE_PROPERTY = "Active";
const std::string OsgParser::DISABLE_PROPERTY = "Disable";

const std::string OsgParser::STR_TRUE = "True";


const std::string OsgParser::myosg_path = "/var/lib/fts3/osg.xml";

OsgParser::OsgParser(std::string path)
{
    doc.load_file(path.c_str());
}


OsgParser::~OsgParser()
{

}


bool OsgParser::isInUse()
{
    static const std::string myosg_str = "myosg";

    std::vector<std::string> providers = ServerConfig::instance().get<std::vector<std::string> >("InfoProviders");
    std::vector<std::string>::iterator it;

    for (it = providers.begin(); it != providers.end(); ++it) {
        if (myosg_str == *it) return true;
    }

    return false;
}


std::string OsgParser::get(std::string fqdn, std::string property)
{
    // if not on the list containing info providers return an empty std::string
    if (!isInUse())
        return std::string();

    // if the MyOSG was set to 'flase' return an empty std::string
    if (!ServerConfig::instance().get<bool>("MyOSG"))
        return std::string();

    // look for the resource name (assume that the user has provided a fqdn)
    xpath_node node = doc.select_single_node(xpath_fqdn(fqdn).c_str());
    std::string val = node.node().child_value(property.c_str());

    if (!val.empty())
        return val;

    // if the name was empty, check if the user provided an fqdn alias
    node = doc.select_single_node(xpath_fqdn_alias(fqdn).c_str());
    return node.node().child_value(property.c_str());
}


std::string OsgParser::getSiteName(std::string fqdn)
{
    return get(fqdn, NAME_PROPERTY);
}


boost::optional<bool> OsgParser::isActive(std::string fqdn)
{
    std::string val = get(fqdn, ACTIVE_PROPERTY);
    if (val.empty())
        return boost::optional<bool>();
    return val == STR_TRUE;
}


boost::optional<bool> OsgParser::isDisabled(std::string fqdn)
{
    std::string val = get(fqdn, DISABLE_PROPERTY);
    if (val.empty())
        return boost::optional<bool>();
    return val == STR_TRUE;
}


std::string OsgParser::xpath_fqdn(std::string fqdn)
{
    static const std::string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDN='";
    static const std::string xpath_end = "']";

    return xpath_fqdn + fqdn + xpath_end;
}


std::string OsgParser::xpath_fqdn_alias(std::string alias)
{
    static const std::string xpath_fqdn = "/ResourceSummary/ResourceGroup/Resources/Resource[FQDNAliases/FQDNAlias='";
    static const std::string xpath_end = "']";

    return xpath_fqdn + alias + xpath_end;
}

} /* namespace cli */
} /* namespace fts3 */
