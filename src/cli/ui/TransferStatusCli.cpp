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
#include <vector>

using namespace fts3::cli;

TransferStatusCli::TransferStatusCli()
{

    // add fts3-transfer-status specific options
    specific.add_options()
    ("list,l", "List status for all files.")
    ("archive,a", "Query the archive.")
    ;
}

TransferStatusCli::~TransferStatusCli()
{
}

optional<GSoapContextAdapter&> TransferStatusCli::validate(bool init)
{

    if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

    if (getJobIds().empty())
        {
            printer().missing_parameter("Request ID");
            return 0;
        }

    return *ctx;
}

bool TransferStatusCli::list()
{

    return vm.count("list");
}

bool TransferStatusCli::queryArchived()
{
    return vm.count("archive");
}
