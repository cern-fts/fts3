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
 * BlacklistCli.cpp
 *
 *  Created on: Nov 7, 2012
 *      Author: simonm
 */

#include "BlacklistCli.h"

#include "exception/bad_option.h"

#include <boost/algorithm/string.hpp>

namespace fts3
{
namespace cli
{

using namespace boost::algorithm;

const string BlacklistCli::ON = "on";
const string BlacklistCli::OFF = "off";

const string BlacklistCli::SE = "se";
const string BlacklistCli::DN = "dn";

BlacklistCli::BlacklistCli()
{

    // add commandline options specific for fts3-set-blk
    hidden.add_options()
    ("type", value<string>(&type), "Specify subject type (se/dn)")
    ("subject", value<string>(&subject), "Subject name.")
    ("mode", value<string>(&mode), "Mode, either: on or off")
    ;

    specific.add_options()
    ("status", value<string>(&status)->default_value("WAIT"), "Status of the jobs that are already in the queue (CANCEL or WAIT)")
    ("timeout", value<int>(&timeout)->default_value(0), "The timeout for the jobs that are already in the queue in case if 'WAIT' status (0 means the job wont timeout)")
    ;

    command_specific.add_options()
    ("vo", value<string>(&vo), "The VO that is banned for the given SE")
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

bool BlacklistCli::validate()
{

    // do the standard validation
    if (!CliBase::validate()) return false;

    to_lower(mode);

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

    return true;
}

string BlacklistCli::getUsageString(string tool)
{
    return "Usage: " + tool + " [options] COMMAND NAME ON|OFF";
}

bool BlacklistCli::printHelp(string tool)
{

    // check whether the -h option was used
    if (vm.count("help"))
        {

            // remove the path to the executive
            size_t pos = tool.find_last_of('/');
            if( pos != string::npos)
                {
                    tool = tool.substr(pos + 1);
                }
            // print the usage guigelines
            cout << endl << getUsageString(tool) << endl << endl;

            cout << "List of Commands:" << endl << endl;
            cout << "dn		Blacklist DN (user)" << endl;
            cout << "se [options]	Blacklist SE (to get further information use '-h')" << endl;
            cout << endl << endl;

            // print the available options
            cout << visible << endl << endl;

            // print SE command options
            if (vm.count("type") && type == "se")
                {
                    cout << "SE options:" << endl << endl;
                    cout << command_specific << endl;
                }

            cout << "Examples: " << endl;
            cout << "\t- To blacklist a SE:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on" << endl;
            cout << "\t- To blacklist a SE for given VO:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on --vo $VO" << endl;
            cout << "\t- To blacklist a SE but still accept transfer-jobs:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE on --allow-submit" << endl;
            cout << "\t- To remove a SE from blacklist:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT se $SE off" << endl;
            cout << "\t- To blacklist a DN:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT dn $DN on" << endl;
            cout << "\t- To remove a DN from blacklist:" << endl;
            cout << "\t  fts-set-blacklist -s $FTSENDPOINT dn $DN off" << endl;

            return true;
        }

    return false;
}

} /* namespace cli */
} /* namespace fts3 */
