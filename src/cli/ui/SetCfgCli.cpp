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

#include "exception/bad_option.h"

#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#endif

using namespace boost::algorithm;
using namespace fts3::cli;

SetCfgCli::SetCfgCli(bool spec)
{
    type = CfgParser::NOT_A_CFG;

    if (spec)
        {
            // add commandline options specific for fts3-config-set
            specific.add_options()
            (
                "bring-online",
                "If this switch is used the user should provide SE_NAME VALUE pairs in order to set the maximum number of files that are staged concurrently for a given SE."
                "\n(Example: --bring-online $SE_NAME $VALUE ...)"
            )
            (
                "drain", value<string>(),
                "If set to 'on' drains the given endpoint."
                "\n(Example: --drain on|off)"
            )
            (
                "retry", value< vector<string> >()->multitoken(),
                "Sets the number of retries of each individual file transfer for the given VO (the value should be greater or equal to -1)."
                "\n(Example: --retry $VO $NB_RETRY)"
            )
            (
                "optimizer-mode", value<int>(),
                "Sets the optimizer mode (allowed values: 1, 2 or 3)"
                "\n(Example: --optimizer-mode 1|2|3)"
            )
            (
                "queue-timeout", value<int>(),
                "Sets the maximum time (in hours) transfer job is allowed to be in the queue (the value should be greater or equal to 0)."
                "\n(Example: --queue-timeout $TIMEOUT)"
            )
            (	"source", value<string>(),
                "The source SE"
                "\n(Example: --source $SE_NAME)"
            )
            (
                "destination", value<string>(),
                "The destination SE"
                "\n(Example: --destination $SE_NAME)"
            )
            (
                "max-bandwidth", value<int>(),
                "The maximum bandwidth that can be used (in MB/s) for the given source or destination (see --source & --destination)"
                "\n(Example: --max-bandwidth $LIMIT)"
            )
            (	"protocol", value< vector<string> >()->multitoken(),
                "Set protocol (UDT) for given SE"
                "\n(Example: --protocol udt $SE_NAME on|off)"
            )
            (
                "max-se-source-active", value< vector<string> >()->multitoken(),
                "Maximum number of active transfers for given source SE (-1 means no limit)"
                "\n(Example: --max-se-source-active $NB_ACT $SE_NAME)"
            )
            (
                "max-se-dest-active", value< vector<string> >()->multitoken(),
                "Maximum number of active transfers for given destination SE (-1 means no limit)"
                "\n(Example: --max-se-dest-active $NB_ACT $SE_NAME)"
            )
            (
                "global-timeout", value<int>(),
                "Global transfer timeout"
                "\n(Example: --global-timeout $TIMEOUT)"
            )
            (
                "sec-per-mb", value<int>(),
                "number of seconds per MB"
                "\n(Example: --sec-per-mb $SEC_PER_MB)"
            )
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
                    throw bad_option("cfg", "Configuration error: most likely you didn't use single quotation marks (') around a configuration!");
                }

            // parse the configuration, check if its valid JSON format, and valid configuration
            CfgParser c(*it);

            type = c.getCfgType();
            if (type == CfgParser::NOT_A_CFG) throw bad_option("cfg", "The specified configuration doesn't follow any of the valid formats!");
        }
}

bool SetCfgCli::validate()
{

    if(!CliBase::validate()) return false;

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
    		throw cli_exception("No parameters have been specified.");
        }

    boost::optional<std::pair<std::string, int>> src = getMaxSeActive("max-se-source-active");
    boost::optional<std::pair<std::string, int>> dst = getMaxSeActive("max-se-dest-active");

    if (src.is_initialized() && dst.is_initialized())
        {
            if (src.get().second != dst.get().second)
                throw bad_option(
                    "max-se-source-active, max-se-dest-active",
                    "the number of active for source and destination has to be equal"
                );
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
            string const & value = vm["drain"].as<string>();

            if (value != "on" && value != "off")
                {
                    throw bad_option("drain", "drain may only take on/off values!");
                }
            else if(!vm.count("service"))
                {
                    throw bad_option("drain", "You need specify which endpoint to drain, -s missing?");
                }

            return vm["drain"].as<string>() == "on";
        }

    return optional<bool>();
}

optional< pair<string, int> > SetCfgCli::retry()
{
    if (vm.count("retry"))
        {
            // get a reference to the options set by the user
            vector<string> const & v = vm["retry"].as< vector<string> >();
            // make sure the number of parameters is correct
            if (v.size() != 2) throw bad_option("retry", "following parameters were expected: VO nb_of_retries");

            try
                {
                    int retries = lexical_cast<int>(v[1]);
                    if (retries < -1) throw bad_option("retry", "incorrect value: the number of retries has to greater or equal to -1.");
                    return make_pair(v[0], retries);
                }
            catch(bad_lexical_cast& ex)
                {
                    throw bad_option("retry", "incorrect value: " + v[1] + " (the number of retries has be an integer).");
                }
        }

    return optional< pair<string, int> >();
}

optional<int> SetCfgCli::optimizer_mode()
{
    if (vm.count("optimizer-mode"))
        {
            int mode = vm["optimizer-mode"].as<int>();

            if (mode < 1 || mode > 3)
                {
                    throw bad_option("optimizer-mode", "only following values are accepted: 1, 2 or 3");
                }

            return mode;
        }

    return optional<int>();
}

optional<unsigned> SetCfgCli::queueTimeout()
{
    if (vm.count("queue-timeout"))
        {
            int timeout = vm["queue-timeout"].as<int>();
            if (timeout < 0) throw bad_option("queue-timeout", "the queue-timeout value has to be greater or equal to 0.");
            return timeout;
        }

    return optional<unsigned>();
}

optional< std::tuple<string, string, string> > SetCfgCli::getProtocol()
{
    // check if the option was used
    if (!vm.count("protocol")) return optional< std::tuple<string, string, string> >();
    // make sure it was used corretly
    const vector<string>& v = vm["protocol"].as< vector<string> >();
    if (v.size() != 3) throw bad_option("protocol", "'--protocol' takes following parameters: udt SE on/off");
    if (v[2] != "on" && v[2] != "off") throw bad_option("protocol", "'--protocol' can only be switched 'on' or 'off'");

    return std::make_tuple(v[0], v[1], v[2]);
}

void SetCfgCli::parseBringOnline()
{

    vector<string> v = vm["cfg"].as< vector<string> >();
    // check if the number of parameters is even
    if (v.size() % 2) throw bad_option("bring-online", "After specifying '--bring-online' SE_NAME - VALUE pairs have to be given!");

    vector<string>::iterator first = v.begin(), second;

    do
        {
            second = first + 1;
            try
                {
                    bring_online.insert(
                        make_pair(*first, lexical_cast<int>(*second))
                    );
                    first += 2;
                }
            catch(boost::bad_lexical_cast const & ex)
                {
                    throw bad_option("bring-online", "The bring-online value: " + *second + " is not a correct integer (int) value!");
                }
        }
    while (first != v.end());
}

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
BOOST_AUTO_TEST_SUITE( cli )
BOOST_AUTO_TEST_SUITE(SetCfgCliTest)

BOOST_AUTO_TEST_CASE (SetCfgCli_bad_lixical_cast)
{
    // test for integer overflow
    char* av[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--bring-online",
        "srm://se",
        "11111111111111111111111111111111111111111111111111111111111111111111111111"
    };


    SetCfgCli* cli = new SetCfgCli();
    BOOST_CHECK_THROW(cli->parse(6, av), bad_option);

    // test for non-numerical characters
    char* av2[] =
    {
        "prog_name",
        "-s",
        "https://fts3-server:8080",
        "--bring-online",
        "srm://se",
        "dadadad"
    };

    cli = new SetCfgCli();
    BOOST_CHECK_THROW(cli->parse(6, av2), bad_option);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
#endif // FTS3_COMPILE_WITH_UNITTESTS

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

    if (v.size() != 2) throw bad_option(option, "'--" + option + "' takes following parameters: number_of_active SE");

    string se = v[1];

    try
        {
            int active = lexical_cast<int>(v[0]);

            if (active < -1) throw bad_option("option", "values lower than -1 are not valid");

            return make_pair(se, active);
        }
    catch(bad_lexical_cast& ex)
        {
            throw bad_option(option, "the number of active has to be an integer");
        }
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

    if (timeout < -1) throw bad_option("global-timeout", "values lower than -1 are not valid");

    if (timeout == -1) timeout = 0;

    return timeout;
}

optional<int> SetCfgCli::getSecPerMb()
{
    if (!vm.count("sec-per-mb")) return optional<int>();

    int sec = vm["sec-per-mb"].as<int>();

    if (sec < -1) throw bad_option("sec-per-mb", "values lower than -1 are not valid");

    if (sec == -1) sec = 0;

    return sec;
}

