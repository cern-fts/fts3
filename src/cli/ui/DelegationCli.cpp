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

#include "DelegationCli.h"

namespace fts3
{
namespace cli
{

DelegationCli::DelegationCli()
{
    // add commandline options specific for fts3-transfer-submit
    specific.add_options()
    ("id,I", po::value<std::string>(), "Delegation with ID as the delegation identifier.")
    ("expire,e", po::value<long>(), "Expiration time of the delegation in seconds.")
    ;
}

DelegationCli::~DelegationCli()
{

}


std::string DelegationCli::getDelegationId() const
{

    // check if destination was passed via command line options
    if (vm.count("id"))
        {
            return vm["id"].as<std::string>();
        }
    return "";
}

long DelegationCli::getExpirationTime() const
{

    if (vm.count("expire"))
        {
            return vm["expire"].as<long>();
        }
    return 0;
}

} /* namespace cli */
} /* namespace fts3 */
