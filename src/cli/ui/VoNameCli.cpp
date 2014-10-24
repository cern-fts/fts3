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
 * ListVOManager.cpp
 *
 *  Created on: Mar 14, 2012
 *      Author: Michal Simon
 */

#include "VoNameCli.h"

#include "exception/cli_exception.h"

using namespace fts3::cli;

VoNameCli::VoNameCli(bool pos): pos(pos)
{

    if (pos)
        {
            // add hidden options (not printed in help)
            hidden.add_options()
            ("voname", value<string>(), "Specify VO name.")
            ;

            // the positional parameter goes to voname
            p.add("voname", 1);

        }
    else
        {
            // add fts3-transfer-status specific options
            specific.add_options()
            ("voname,o", value<string>(), "Restrict to specific VO")
            ;
        }
}

VoNameCli::~VoNameCli()
{
}

void VoNameCli::validate()
{
    if (pos)
        {
            if (getVoName().empty())
                {
                    throw cli_exception("The VO name has to be specified");
                }
        }
}

string VoNameCli::getUsageString(string tool)
{
    return "Usage: " + tool + " [options] VONAME";
}

string VoNameCli::getVoName()
{

    // check whether jobid has been given as a parameter
    if (vm.count("voname"))
        {
            return vm["voname"].as<string>();
        }

    return string();
}

