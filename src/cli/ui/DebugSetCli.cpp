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
 * DebugSetCli.h
 *
 *  Created on: Aug 2, 2012
 *      Author: Michał Simon
 */

#include "DebugSetCli.h"

#include "exception/cli_exception.h"

using namespace fts3::cli;

const string DebugSetCli::ON = "on";
const string DebugSetCli::OFF = "off";

DebugSetCli::DebugSetCli()
{
    mode = false;

    // add hidden options
    hidden.add_options()
    ("opt1", value<string>())
    ("opt2", value<string>())
    ("opt3", value<string>())
    ;

    // add positional (those used without an option switch) command line options
    p.add("opt1", 1);
    p.add("opt2", 1);
    p.add("opt3", 1);
}

DebugSetCli::~DebugSetCli()
{

}

bool DebugSetCli::validate()
{

    // do the standard validation
    if(!CliBase::validate()) return false;

    vector<string> opts;

    if (vm.count("opt1"))
        {
            opts.push_back(
                vm["opt1"].as<string>()
            );
        }

    if (vm.count("opt2"))
        {
            opts.push_back(
                vm["opt2"].as<string>()
            );
        }

    if (vm.count("opt3"))
        {
            opts.push_back(
                vm["opt3"].as<string>()
            );
        }

    // make sure that at least one SE and debug mode were specified
    if (opts.size() < 2)
        {
    		throw cli_exception("SE name and debug mode has to be specified (on/off)!");
        }

    // index of debug mode (the last parameter)
    size_t mode_index = opts.size() - 1;
    // value of debug mode
    string mode_str = opts[mode_index];
    // it should be either ON
    if (mode_str == ON) mode = true;
    // or OFF
    else if (mode_str == OFF) mode = false;
    // otherwise it's an error
    else
        {
    		throw cli_exception("Debug mode has to be specified (on/off)!");
        }

    // source is always the first one
    source = opts[0];

    // if mode is the second parameter the destination was not specified!
    if (mode_index > 1) destination = opts[1];

    return true;
}

string DebugSetCli::getUsageString(string tool)
{
    return "Usage: " + tool + " [options] (SE | SOURCE DESTINATION) MODE";
}
