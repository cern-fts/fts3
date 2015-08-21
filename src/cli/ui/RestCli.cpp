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

#include "RestCli.h"

#include "exception/bad_option.h"

using namespace fts3::cli;

RestCli::RestCli()
{
    // add fts3-transfer-status specific options
    specific.add_options()
    ("rest", "Use the RESTful interface.")
    ("capath", po::value<std::string>(),  "Path to the GRID security certificates (e.g. /etc/grid-security/certificates).")
    ;
}

RestCli::~RestCli()
{

}

bool RestCli::rest() const
{
    // return true if rest is in the map
    return vm.count("rest");
}

std::string RestCli::capath() const
{
    if (vm.count("capath"))
        {
            return vm["capath"].as<std::string>();
        }

    return "/etc/grid-security/certificates";
}
