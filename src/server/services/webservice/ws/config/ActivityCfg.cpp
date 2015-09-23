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

#include "ActivityCfg.h"

namespace fts3
{
namespace ws
{


ActivityCfg::ActivityCfg(std::string dn, std::string name) : Configuration(dn), vo(name)
{
    init(vo);
}

ActivityCfg::ActivityCfg(std::string dn, CfgParser& parser) : Configuration(dn)
{
    vo = parser.get<std::string>("vo");
    active = parser.get<bool>("active");
    shares = parser.get< std::map<std::string, double> >("share");

    all = json();
}

ActivityCfg::~ActivityCfg()
{

}

std::string ActivityCfg::json()
{

    std::stringstream ss;

    ss << "{";
    ss << "\"" << "vo" << "\":\"" << vo << "\",";
    ss << "\"" << "active" << "\":" << (active ? "true" : "false") << ",";
    ss << "\"" << "share" << "\":" << Configuration::json(shares);
    ss << "}";

    return ss.str();
}

void ActivityCfg::save()
{
    if (db->getActivityConfig(vo).empty())
        {
            // insert
            db->addActivityConfig(vo, Configuration::json(shares), active);

        }
    else
        {
            // update
            db->updateActivityConfig(vo, Configuration::json(shares), active);
        }
}

void ActivityCfg::del()
{
    db->deleteActivityConfig(vo);
}

void ActivityCfg::init(std::string vo)
{
    active = db->isActivityConfigActive(vo);
    shares = db->getActivityConfig(vo);

    if (shares.empty())
        throw Err_Custom("There is no activity configuration for: " + vo);
}

} /* namespace ws */
} /* namespace fts3 */
