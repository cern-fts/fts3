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

#include "PairCfg.h"

#include <sstream>

#include <utility>

namespace fts3
{
namespace ws
{

PairCfg::PairCfg(string dn, string source, string destination) :
    Configuration(dn),
    source(source),
    destination(destination)
{

    notAllowed.insert(any);
    if (notAllowed.count(source) || notAllowed.count(destination))
        throw Err_Custom("The source or destination name is not a valid!");

    std::unique_ptr<LinkConfig> cfg (
        db->getLinkConfig(source, destination)
    );

    if (!cfg.get())
        throw Err_Custom("A configuration for " + source + " - " + destination + " pair does not exist!");

    symbolic_name = cfg->symbolic_name;
    active = cfg->state == on;

    share = getShareMap(source, destination);
    protocol = getProtocolMap(cfg.get());
}

PairCfg::PairCfg(string dn, CfgParser& parser) : Configuration(dn)
{

    notAllowed.insert(any);

    symbolic_name_opt = parser.get_opt("symbolic_name");
    share = parser.get< map<string, int> >("share");
    if (!parser.isAuto("protocol"))
        protocol = parser.get< map<string, int> >("protocol");
    active = parser.get<bool>("active");
}

PairCfg::~PairCfg()
{
}

string PairCfg::json()
{

    stringstream ss;

    ss << "\"" << "symbolic_name" << "\":\"" << symbolic_name << "\",";
    ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
    ss << "\"" << "share" << "\":" << Configuration::json(share) << ",";
    ss << "\"" << "protocol" << "\":" << Configuration::json(protocol);

    return ss.str();
}

void PairCfg::save()
{
    // add link
    addLinkCfg(source, destination, active, symbolic_name, protocol);
    // add shres for the link
    addShareCfg(source, destination, share);
}

void PairCfg::del()
{
    // delete shares
    delShareCfg(source, destination);
    // delete the link itself
    delLinkCfg(source, destination);
}

} /* namespace ws */
} /* namespace fts3 */
