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

#include "GrPairCfg.h"

namespace fts3
{
namespace ws
{

GrPairCfg::GrPairCfg(string dn, CfgParser& parser) : PairCfg(dn, parser)
{

    source = parser.get<string>("source_group");
    destination = parser.get<string>("destination_group");

    if (notAllowed.count(source) || notAllowed.count(destination))
        throw Err_Custom("The source or destination name is not a valid!");

    if (symbolic_name_opt)
        symbolic_name = *symbolic_name_opt;
    else
        symbolic_name = source + "-" + destination;


    all = json();
}

GrPairCfg::~GrPairCfg()
{
}

string GrPairCfg::json()
{

    stringstream ss;

    ss << "{";
    ss << "\"" << "source_group" << "\":\"" << source << "\",";
    ss << "\"" << "destination_group" << "\":\"" << destination << "\",";
    ss << PairCfg::json();
    ss << "}";

    return ss.str();
}

void GrPairCfg::save()
{
    checkGroup(source);
    checkGroup(destination);
    PairCfg::save();
}

} /* namespace ws */
} /* namespace fts3 */
