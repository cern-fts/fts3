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

#include "BlacklistCli.h"

#include "exception/bad_option.h"

#include <boost/algorithm/string.hpp>

namespace fts3
{
namespace cli
{

const std::string BlacklistCli::ON = "on";
const std::string BlacklistCli::OFF = "off";

const std::string BlacklistCli::SE = "se";
const std::string BlacklistCli::DN = "dn";

BlacklistCli::BlacklistCli()
{

    // add commandline options specific for fts3-set-blk
    hidden.add_options()
    ("type", po::value<std::string>(&type), "Specify subject type (se/dn)")
    ("subject", po::value<std::string>(&subject), "Subject name.")
    ("mode", po::value<std::string>(&mode), "Mode, either: on or off")
    ;

    specific.add_options()
    ("status", po::value<std::string>(&status)->default_value("WAIT"), "Status of the jobs that are already in the queue (CANCEL or WAIT)")
    ("timeout", po::value<int>(&timeout)->default_value(0), "The timeout for the jobs that are already in the queue in case if 'WAIT' status (0 means the job wont timeout)")
    ;

    command_specific.add_options()
    ("vo", po::value<std::string>(&vo), "The VO that is banned for the given SE")
    ("allow-submit", "FTS will accept transfer jobs for the blacklisted SE (they wont be executed until the SE is blacklisted)")
    ;

    // add positional (those used without an option switch) command line options
    p.add("type", 1);
    p.add("subject", 1);
    p.add("mode", 1);
}

BlacklistCli::~BlacklistCli()
{

}

void BlacklistCli::validate()
{
    boost::algorithm::to_lower(mode);

    if (mode != ON && mode != OFF)
        {
            throw bad_option("mode", "has to be either 'on' or 'off'");
        }

    if (type != SE && type != DN)
        {
            throw bad_option("type", "has to be either 'se' or 'dn'");
        }


    if ( (!vm.count("status") || status != "WAIT") && timeout != 0)
        {
            throw bad_option("timeout", "may be only specified if status = 'WAIT'");
        }

    if (vm.count("allow-submit") && status == "CANCEL")
        {
            throw bad_option("allow-submit", "may not be used if status = 'CANCEL'");
        }
}

std::string BlacklistCli::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options] COMMAND NAME ON|OFF";
}

bool BlacklistCli::printHelp() const
{
    // check whether the -h option was used
    if (vm.count("help"))
        {

            // remove the path to the executive
            std::string basename(toolname);
            size_t pos = basename.find_last_of('/');
            if( pos != std::string::npos)
                {
                    basename = basename.substr(pos + 1);
                }
            // print the usage guigelines
            std::cout << std::endl << getUsageString(basename) << std::endl << std::endl;

            std::cout << "List of Commands:" << std::endl << std::endl;
            std::cout << "dn        Blacklist DN (user)" << std::endl;
            std::cout << "se [options]  Blacklist SE (to get further information use '-h')" << std::endl;
            std::cout << std::endl << std::endl;

            // print the available options
            std::cout << visible << std::endl << std::endl;

            // print SE command options
            if (vm.count("type") && type == "se")
                {
                    std::cout << "SE options:" << std::endl << std::endl;
                    std::cout << command_specific << std::endl;
                }

            std::cout << "Examples: " << std::endl;
            std::cout << "\t- To blacklist a SE:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on" << std::endl;
            std::cout << "\t- To blacklist a SE for given VO:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on --vo $VO" << std::endl;
            std::cout << "\t- To blacklist a SE but still accept transfer-jobs:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on --allow-submit" << std::endl;
            std::cout << "\t- To remove a SE from blacklist:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE off" << std::endl;
            std::cout << "\t- To blacklist a DN:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT dn $DN on" << std::endl;
            std::cout << "\t- To remove a DN from blacklist:" << std::endl;
            std::cout << "\t  fts-set-blacklist -s $FTSENDPOINT dn $DN off" << std::endl;

            return true;
        }

    return false;
}

} /* namespace cli */
} /* namespace fts3 */
