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

#include "StandaloneSeCfg.h"

#include <sstream>

namespace fts3
{
namespace ws
{

using namespace boost;

StandaloneSeCfg::StandaloneSeCfg(std::string dn, std::string name) : StandaloneCfg(dn), se(name)
{
    if (notAllowed.count(se))
        throw Err_Custom("The SE name is not a valid!");

    // replace any with wildcard
    if (se == any) se = wildcard;

    // get SE active state
    std::unique_ptr<LinkConfig> ptr (db->getLinkConfig(se, "*"));
    if (ptr.get())
        active = ptr->state == on;
    else
        throw Err_Custom("The SE: " + name + " does not exist!");

    init(se);
}

StandaloneSeCfg::StandaloneSeCfg(std::string dn, CfgParser& parser) : StandaloneCfg(dn, parser)
{
    se = parser.get<std::string>("se");
    all = json();

    if (notAllowed.count(se))
        throw Err_Custom("The SE name is not a valid!");

    // replace any with wildcard
    if (se == any) se = wildcard;
}

StandaloneSeCfg::~StandaloneSeCfg()
{
}

std::string StandaloneSeCfg::json()
{
    std::stringstream ss;

    ss << "{";
    ss << "\"" << "se" << "\":\"" << (se == wildcard ? any : se) << "\",";
    ss << StandaloneCfg::json();
    ss << "}";

    return ss.str();
}

void StandaloneSeCfg::save()
{
    addSe(se, active);
    StandaloneCfg::save(se);
}

void StandaloneSeCfg::del()
{
    eraseSe(se);
    StandaloneCfg::del(se);
}

} /* namespace common */
} /* namespace fts3 */
