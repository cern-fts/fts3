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

#include "ListTransferCli.h"

using namespace fts3::cli;


ListTransferCli::ListTransferCli(): VoNameCli(false)
{

    // add hidden options (not printed in help)
    CliBase::hidden.add_options()
    ("state", po::value< std::vector<std::string> >(), "Specify states for querying.")
    ;

    // add fts3-transfer-list specific options
    specific.add_options()
    ("source_se", po::value<std::string>(), "Restrict to specific source SE.")
    ("dest_se", po::value<std::string>(), "Restrict to specific destination SE.")
    ("deletion", "Query for deletion jobs. (Has no effect when using REST)")
    ;

    // all positional parameters go to state
    CliBase::p.add("state", -1);
}

ListTransferCli::~ListTransferCli()
{
}

void ListTransferCli::validate()
{
    // check for invalid options according to the base clases
    DnCli::validate();
    VoNameCli::validate();
    TransferCliBase::validate();
}

std::string ListTransferCli::getUsageString(std::string tool) const
{

    return "Usage: " + tool + " [options] [STATE...]";
}

std::vector<std::string> ListTransferCli::getStatusArray()
{

    std::vector<std::string> array;

    if (CliBase::vm.count("state"))
        {
            array = CliBase::vm["state"].as< std::vector<std::string> >();
        }

    if (array.empty())
        {
            if (deletion())
                array.push_back("DELETE");
            else
                array.push_back("SUBMITTED");
            array.push_back("ACTIVE");
            array.push_back("READY");
        }

    return array;
}

std::string ListTransferCli::source()
{
    if (vm.count("source_se"))
        {
            return vm["source_se"].as<std::string>();
        }

    return std::string();
}

std::string ListTransferCli::destination()
{
    if (vm.count("dest_se"))
        {
            return vm["dest_se"].as<std::string>();
        }

    return std::string();
}

bool ListTransferCli::deletion()
{
    return vm.count("deletion");
}
