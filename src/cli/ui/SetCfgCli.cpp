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
 * NameValueCli.cpp
 *
 *  Created on: Apr 2, 2012
 *      Author: Michał Simon
 */

#include "SetCfgCli.h"

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::algorithm;
using namespace fts3::cli;

SetCfgCli::SetCfgCli(bool spec)
{

    if (spec)
        {
            // add commandline options specific for fts3-transfer-submit
            specific.add_options()
            ("bring-online", "If this switch is used the user should provide SE_NAME VALUE pairs in order to set the maximum number of files that are staged concurrently for a given SE.")
            ("drain", value<string>(), "If set to 'on' drains the given endpoint.")
            ("retry", value<int>(), "Sets the number of retries of each individual file transfer (the value should be greater or equal to -1).")
            ("optimizer-mode", value<int>(), "Sets the optimizer mode (allowed values: 1, 2 or 3)")
            ("queue-timeout", value<int>(), "Sets the maximum time (in hours) transfer job is allowed to be in the queue (the value should be greater or equal to 0).")
            ;
        }

    // add hidden options (not printed in help)
    hidden.add_options()
    ("cfg", value< vector<string> >(), "Specify SE configuration.")
    ;

    // all positional parameters go to jobid
    p.add("cfg", -1);
}

SetCfgCli::~SetCfgCli()
{
}

void SetCfgCli::parse(int ac, char* av[])
{

    // do the basic initialization
    CliBase::parse(ac, av);

    if (vm.count("drain"))
        {
            if (vm["drain"].as<string>() != "on" && vm["drain"].as<string>() != "off")
                {
                    throw string("drain may only take on/off values!");
                }
        }

    if (vm.count("cfg"))
        {
            if (vm.count("bring-online"))
                parseBringOnline();
            else
                cfgs = vm["cfg"].as< vector<string> >();
        }

    // check JSON configurations
    vector<string>::iterator it;
    for (it = cfgs.begin(); it < cfgs.end(); it++)
        {
            trim(*it);
            // check if the configuration is started with an opening brace and ended with a closing brace
            if (*it->begin() != '{' || *(it->end() - 1) != '}')
                {
                    // most likely the user didn't used single quotation marks and bash did some pre-parsing
                    throw string("Configuration error: most likely you didn't use single quotation marks (') around a configuration!");
                }

            // parse the configuration, check if its valid JSON format, and valid configuration
            CfgParser c(*it);

            type = c.getCfgType();
            if (type == CfgParser::NOT_A_CFG) throw string("The specified configuration doesn't follow any of the valid formats!");
        }
}

optional<GSoapContextAdapter&> SetCfgCli::validate(bool init)
{

    if (!CliBase::validate(init).is_initialized()) return optional<GSoapContextAdapter&>();

    if (vm.count("retry"))
        {
            int val = vm["retry"].as<int>();
            if (val < -1)
                {
                    cout << "The retry value has to be greater or equal to -1." << endl;
                    return optional<GSoapContextAdapter&>();
                }
        }

    if (vm.count("queue-timeout"))
        {
            int val = vm["queue-timeout"].as<int>();
            if (val < 0)
                {
                    cout << "The queue-timeout value has to be greater or equal to 0." << endl;
                    return optional<GSoapContextAdapter&>();
                }
        }

    if (vm.count("optimizer-mode"))
        {
            int val = vm["optimizer-mode"].as<int>();
            if (val < 1 || val > 3)
                {
                    cout << "optimizer-mode may only take following values: 1, 2 or 3" << endl;
                    return optional<GSoapContextAdapter&>();
                }
        }

    if (getConfigurations().empty()
            && !vm.count("drain")
            && !vm.count("retry")
            && !vm.count("queue-timeout")
            && !vm.count("bring-online")
            && !vm.count("optimizer-mode")
       )
        {
            cout << "No parameters have been specified." << endl;
            return optional<GSoapContextAdapter&>();
        }

    return *ctx;
}

string SetCfgCli::getUsageString(string tool)
{
    return "Usage: " + tool + " [options] CONFIG [CONFIG...]";
}

vector<string> SetCfgCli::getConfigurations()
{
    return cfgs;
}

optional<bool> SetCfgCli::drain()
{

    if (vm.count("drain"))
        {
            return vm["drain"].as<string>() == "on";
        }

    return optional<bool>();
}

optional<int> SetCfgCli::retry()
{
    if (vm.count("retry"))
        {
            return vm["retry"].as<int>();
        }

    return optional<int>();
}

optional<int> SetCfgCli::optimizer_mode()
{
    if (vm.count("optimizer-mode"))
        {
            return vm["optimizer-mode"].as<int>();
        }

    return optional<int>();
}

optional<unsigned> SetCfgCli::queueTimeout()
{

    if (vm.count("queue-timeout"))
        {
            return vm["queue-timeout"].as<int>();
        }

    return optional<unsigned>();
}

void SetCfgCli::parseBringOnline()
{

    vector<string> v = vm["cfg"].as< vector<string> >();
    // check if the number of parameters is even
    if (v.size() % 2) throw string("After specifying '--bring-online' SE_NAME - VALUE pairs have to be given!");

    vector<string>::iterator first = v.begin(), second;

    do
        {
            second = first + 1;
            bring_online.insert(
                make_pair(*first, lexical_cast<int>(*second))
            );
            first += 2;
        }
    while (first != v.end());
}

map<string, int> SetCfgCli::getBringOnline()
{

    return bring_online;
}
