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

#include "StandaloneGrCfg.h"

namespace fts3
{
namespace ws
{

StandaloneGrCfg::StandaloneGrCfg(string dn, string name) : StandaloneCfg(dn), group(name)
{

    notAllowed.insert(any);
    if (notAllowed.count(group))
        throw Err_Custom("The SE name is not a valid!");

    if (!db->checkGroupExists(group))
        throw Err_Custom("The SE group: " + group + " does not exist!");

    active = true;

    // init shares and protocols
    init(name);

    // get group members
    db->getGroupMembers(name, members);
}

StandaloneGrCfg::StandaloneGrCfg(string dn, CfgParser& parser) : StandaloneCfg(dn, parser)
{

    notAllowed.insert(any);

    group = parser.get<string>("group");
    members = parser.get< vector<string> >("members");

    all = json();

    if (notAllowed.count(group))
        throw Err_Custom("The SE name is not a valid!");
}

StandaloneGrCfg::~StandaloneGrCfg()
{
}

string StandaloneGrCfg::json()
{

    stringstream ss;

    ss << "{";
    ss << "\"" << "group" << "\":\"" << group << "\",";
    ss << "\"" << "members" << "\":" << Configuration::json(members) << ",";
    ss << StandaloneCfg::json();
    ss << "}";

    return ss.str();
}

void StandaloneGrCfg::save()
{

    addGroup(group, members);
    StandaloneCfg::save(group);
}

void StandaloneGrCfg::del()
{

    // check if pair configuration uses the group
    if (db->isGrInPair(group))
        throw Err_Custom("The group is used in a group-pair configuration, you need first to remove the pair!");

    // delete group
    StandaloneCfg::del(group);

    // remove group members
    db->deleteMembersFromGroup(group, members);
    deleteCount++;
}

} /* namespace common */
} /* namespace fts3 */
