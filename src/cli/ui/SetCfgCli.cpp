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
    type = CfgParser::NOT_A_CFG;

    if (spec)
        {
            // add commandline options specific for fts3-transfer-submit
            specific.add_options()
            ("bring-online", "If this switch is used the user should provide SE_NAME VALUE pairs in order to set the maximum number of files that are staged concurrently for a given SE.")
            ("drain", value<string>(), "If set to 'on' drains the given endpoint.")
            ("retry", value<int>(), "Sets the number of retries of each individual file transfer (the value should be greater or equal to -1).")
            ("optimizer-mode", value<int>(), "Sets the optimizer mode (allowed values: 1, 2 or 3)")
            ("queue-timeout", value<int>(), "Sets the maximum time (in hours) transfer job is allowed to be in the queue (the value should be greater or equal to 0).")
            ("source", value<string>(), "The source SE")
            ("destination", value<string>(), "The destination SE")
            ("max-bandwidth", value<int>(), "The maximum bandwidth that can be used (in Mbit/s)")
            ("protocol", value< vector<string> >()->multitoken(), "Set protocol (UDT) for given SE")
            ("max-se-source-active", value< vector<string> >()->multitoken(), "Maximum number of active transfers for given source SE")
            ("max-se-dest-active", value< vector<string> >()->multitoken(), "Maximum number of active transfers for given destination SE")
            ("global-timeout", value<int>(), "Global transfer timeout")
            ("sec-per-mb", value<int>(), "number of seconds per MB")
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
    else if(vm.count("max-bandwidth"))
        {
            parseMaxBandwidth();
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

bool SetCfgCli::validate()
{

    if(!CliBase::validate()) return false;

    if (vm.count("retry"))
        {
            int val = vm["retry"].as<int>();
            if (val < -1)
                {
                    msgPrinter.error_msg("The retry value has to be greater or equal to -1.");
                    return false;
                }
        }

    if (vm.count("queue-timeout"))
        {
            int val = vm["queue-timeout"].as<int>();
            if (val < 0)
                {
                    msgPrinter.error_msg("The queue-timeout value has to be greater or equal to 0.");
                    return false;
                }
        }

    if (vm.count("optimizer-mode"))
        {
            int val = vm["optimizer-mode"].as<int>();
            if (val < 1 || val > 3)
                {
                    msgPrinter.error_msg("optimizer-mode may only take following values: 1, 2 or 3");
                    return false;
                }
        }

    if (getConfigurations().empty()
            && !vm.count("drain")
            && !vm.count("retry")
            && !vm.count("queue-timeout")
            && !vm.count("bring-online")
            && !vm.count("optimizer-mode")
            && !vm.count("max-bandwidth")
            && !vm.count("protocol")
            && !vm.count("max-se-source-active")
            && !vm.count("max-se-dest-active")
            && !vm.count("global-timeout")
            && !vm.count("sec-per-mb")
       )
        {
            msgPrinter.error_msg("No parameters have been specified.");
            return false;
        }

    return true;
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

optional< std::tuple<string, string, string> > SetCfgCli::getProtocol()
{
    // check if the option was used
    if (!vm.count("protocol")) return optional< std::tuple<string, string, string> >();
    // make sure it was used corretly
    const vector<string>& v = vm["protocol"].as< vector<string> >();
    if (v.size() != 3) throw string("'--protocol' takes following parameters: udt SE on/off");
    if (v[2] != "on" && v[2] != "off") throw string("'--protocol' can only be switched 'on' or 'off'");

    return std::make_tuple(v[0], v[1], v[2]);
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

void SetCfgCli::parseMaxBandwidth()
{
    std::string source_se, dest_se;

    if (!vm["source"].empty())
        source_se = vm["source"].as<string>();
    if (!vm["destination"].empty())
        dest_se = vm["destination"].as<string>();

    int limit = vm["max-bandwidth"].as<int>();

    bandwidth_limitation = make_optional(std::tuple<string, string, int>(source_se, dest_se, limit));
}

map<string, int> SetCfgCli::getBringOnline()
{
    return bring_online;
}

optional<std::tuple<string, string, int> > SetCfgCli::getBandwidthLimitation()
{
    return bandwidth_limitation;
}

optional< pair<string, int> > SetCfgCli::getMaxSeActive(string option)
{
	if (!vm.count(option))
		{
			return optional< pair<string, int> >();
		}

    // make sure it was used corretly
    const vector<string>& v = vm[option].as< vector<string> >();

    if (v.size() != 2) throw string("'--" + option + "' takes following parameters: SE number_of_active");

	string se = v[0];
	int active = lexical_cast<int>(v[1]);

	if (active < -1) throw string("values lower than -1 are not valid");

	return make_pair(se, active);
}

optional< pair<string, int> > SetCfgCli::getMaxSrcSeActive()
{
	return getMaxSeActive("max-se-source-active");
}

optional< pair<string, int> > SetCfgCli::getMaxDstSeActive()
{
	return getMaxSeActive("max-se-dest-active");
}

optional<int> SetCfgCli::getGlobalTimeout()
{
	if (!vm.count("global-timeout")) return optional<int>();

	int timeout = vm["global-timeout"].as<int>();

	if (timeout < -1) throw string("values lower than -1 are not valid");

	if (timeout == -1) timeout = 0;

	return timeout;
}

optional<int> SetCfgCli::getSecPerMb()
{
	if (!vm.count("sec-per-mb")) return optional<int>();

	int sec = vm["sec-per-mb"].as<int>();

	if (sec < -1) throw string("values lower than -1 are not valid");

	if (sec == -1) sec = 0;

	return sec;
}

