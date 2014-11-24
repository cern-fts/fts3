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
 */

/*
 * TransferStatusCli.cpp
 *
 *  Created on: Feb 13, 2012
 *      Author: simonm
 */

#include "TransferStatusCli.h"

#include "exception/bad_option.h"

#include <vector>

using namespace fts3::cli;

TransferStatusCli::TransferStatusCli()
{

    // add fts3-transfer-status specific options
    specific.add_options()
    ("list,l", "List status for all files.")
    ("p,p", "Get detailed status including file ID (JSON format only)")
    ("archive,a", "Query the archive.")
    ("detailed,d", "Retrieve details (i.e. transfer retries)")
    ("dump-failed,F", "Dump failed transfers into a file that can be used as input for submission")
    ;
}

TransferStatusCli::~TransferStatusCli()
{
}

void TransferStatusCli::validate()
{
    if (getJobIds().empty()) throw bad_option("jobid", "missing parameter");

    if (vm.count("p") && vm.size() > 3)
        {
            po::variables_map::iterator it;
            for (it = vm.begin(); it != vm.end(); ++it)
                {
                    if (it->first == "p") continue;

                    if (it->first != "service" && it->first != "rest"
                            && it->first != "capath" && it->first != "proxy" && it->first != "jobid")
                        throw bad_option("p", "this option cannot be used together with '" + it->first + "'!");
                }
        }
}

bool TransferStatusCli::list()
{

    return vm.count("list");
}

bool TransferStatusCli::queryArchived()
{
    return vm.count("archive");
}


bool TransferStatusCli::dumpFailed()
{
    return vm.count("dump-failed");
}


bool TransferStatusCli::detailed()
{
    return vm.count("detailed");
}

bool TransferStatusCli::p()
{
    return vm.count("p");
}
