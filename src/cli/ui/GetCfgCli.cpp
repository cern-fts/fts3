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

#include "GetCfgCli.h"

using namespace fts3::cli;

GetCfgCli::GetCfgCli() : SrcDestCli(true)
{

    specific.add_options()
    ("name,n", po::value<std::string>(), "Restrict to specific symbolic (configuration) name.")
    ("all", "Get all the configurations (standalone and pairs) for the given SE.")
    ("vo", "Get activity share configuration for the given VO.")
    ("max-bandwidth", "Get bandwidth limitation for a given SE")
    ;
}

GetCfgCli::~GetCfgCli()
{
}

std::string GetCfgCli::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options] [STANDALONE_CFG | SOURCE DESTINATION]";
}

std::string GetCfgCli::getName()
{

    if (vm.count("name"))
        {
            return vm["name"].as<std::string>();
        }

    return std::string();
}

bool GetCfgCli::all()
{
    return vm.count("all");
}

bool GetCfgCli::vo()
{
    return vm.count("vo");
}

bool GetCfgCli::getBandwidth()
{
    return vm.count("max-bandwidth");
}
