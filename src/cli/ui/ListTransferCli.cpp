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
 * ListTransferCli.cpp
 *
 *  Created on: Mar 1, 2012
 *      Author: Michal Simon
 */

#include "ListTransferCli.h"
#include "common/JobStatusHandler.h"

using namespace fts3::cli;
using namespace fts3::common;

ListTransferCli::ListTransferCli(): VoNameCli(false)
{

    // add hidden options (not printed in help)
    CliBase::hidden.add_options()
    ("state", value< vector<string> >(), "Specify states for querying.")
    ;

    // add fts3-transfer-list specific options
    specific.add_options()
    ("source_se", value<string>(), "Restrict to specific source SE.")
    ("dest_se", value<string>(), "Restrict to specific destination SE.")
    ;

    // all positional parameters go to state
    CliBase::p.add("state", -1);
}

ListTransferCli::~ListTransferCli()
{
}

string ListTransferCli::getUsageString(string tool)
{

    return "Usage: " + tool + " [options] [STATE...]";
}

vector<string> ListTransferCli::getStatusArray()
{

    vector<string> array;

    if (CliBase::vm.count("state"))
        {
            array = CliBase::vm["state"].as< vector<string> >();
        }

    if (array.empty())
        {
            array.push_back(JobStatusHandler::FTS3_STATUS_SUBMITTED);
            array.push_back(JobStatusHandler::FTS3_STATUS_ACTIVE);
            array.push_back(JobStatusHandler::FTS3_STATUS_READY);
        }

    return array;
}

string ListTransferCli::source()
{
    if (vm.count("source_se"))
        {
            return vm["source_se"].as<string>();
        }

    return string();
}

string ListTransferCli::destination()
{
    if (vm.count("dest_se"))
        {
            return vm["dest_se"].as<string>();
        }

    return string();
}
