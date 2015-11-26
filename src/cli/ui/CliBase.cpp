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

#include "CliBase.h"

#include <iostream>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "version.h"

#include "exception/bad_option.h"

using namespace fts3::cli;

const std::string CliBase::error = "error";
const std::string CliBase::result = "result";
const std::string CliBase::parameter_error = "parameter_error";

CliBase::CliBase(): visible("Allowed options")
{

    // add basic command line options
    basic.add_options()
    ("help,h", "Print this help text and exit.")
    ("quiet,q", "Quiet operation.")
    ("verbose,v", "Be more verbose.")
    ("service,s", po::value<std::string>(), "Use the transfer service at the specified URL.")
    ("proxy", po::value<std::string>(),  "Path to the proxy certificate (e.g. /tmp/x509up_u500).")
    ("version,V", "Print the version number and exit.")
    ;

    version = getCliVersion();
    interface = version;
}

void CliBase::parse(int ac, char* av[])
{

    // set the output parameters (verbose and json) before the actual parsing happens
    for (int i = 0; i < ac; i++)
        {
            std::string str(av[i]);
            if (str == "-v")
                {
                    MsgPrinter::instance().setVerbose(true);
                }
            else if (str == "-j")
                {
                    MsgPrinter::instance().setJson(true);
                }
        }

    toolname = av[0];

    // add specific and hidden parameters to all parameters
    all.add(basic).add(specific).add(hidden).add(command_specific);
    // add specific parameters to visible parameters (printed in help)
    visible.add(basic).add(specific);

    // turn off guessing, so --source is not mistaken with --source-token
    int style = po::command_line_style::default_style & ~po::command_line_style::allow_guessing;

    // parse the options that have been used
    po::store(po::command_line_parser(ac, av).options(all).positional(p).style(style).run(), vm);
    notify(vm);

    // check is the output is verbose
    MsgPrinter::instance().setVerbose(vm.count("verbose"));
    // check if the output is in json format
    MsgPrinter::instance().setJson(vm.count("json"));

    // check whether the -s option has been used
    const char* fts3_env;

    if (vm.count("service"))
        {
            endpoint = vm["service"].as<std::string>();
            // check if the endpoint has the right prefix
            if (endpoint.find("http") != 0 && endpoint.find("https") != 0 && endpoint.find("httpd") != 0)
                {
                    std::string msg =  "wrong endpoint format ('" + endpoint + "')";
                    throw bad_option("service", msg);
                }
        }
    else if ((fts3_env = getenv("FTS3_ENDPOINT")) != NULL)
        {
            endpoint = fts3_env;
        }
    else
        {

            if (useSrvConfig())
                {
                    std::fstream  cfg ("/etc/fts3/fts3config",std::fstream::in);
                    if (cfg.is_open())
                        {

                            std::string ip;
                            std::string port;
                            std::string line;

                            do
                                {
                                    getline(cfg, line);
                                    if (line.find("#") != 0 && line.find("Port") == 0)
                                        {
                                            // erase 'Port='
                                            port = line.erase(0, 5);

                                        }
                                    else if (line.find("#") != 0 && line.find("IP") == 0)
                                        {
                                            // erase 'IP='
                                            ip += line.erase(0, 3);
                                        }

                                }
                            while(!cfg.eof());

                            if (!ip.empty() && !port.empty())
                                endpoint = "https://" + ip + ":" + port;
                        }
                }
            else if (!vm.count("help") && !vm.count("version"))
                {
                    // if the -s option has not been used try to discover the endpoint
                    // (but only if -h and -v option were not used)
                    endpoint = discoverService();
                }
        }
}

void CliBase::validate()
{
    // if endpoint could not be determined, we cannot do anything
    if (endpoint.empty())
        {
            throw bad_option("service", "failed to determine the endpoint");
        }
}

void CliBase::printCliDetails() const
{
    MsgPrinter::instance().print_info("# Client version", "client_version", version);
    MsgPrinter::instance().print_info("# Client interface version", "client_interface", interface);
}

void CliBase::printApiDetails(ServiceAdapter & ctx) const
{
    if (!isVerbose()) return;

    ctx.printServiceDetails();
    printCliDetails();
}

std::string CliBase::getUsageString(std::string tool) const
{
    return "Usage: " + tool + " [options]";
}

bool CliBase::printHelp() const
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
            // print the available options
            std::cout << visible << std::endl;
            return true;
        }

    // check whether the -V option was used
    if (vm.count("version"))
        {
            MsgPrinter::instance().print("client_version", version);
            return true;
        }

    return false;
}

bool CliBase::isVerbose() const
{
    return vm.count("verbose");
}

bool CliBase::isQuiet() const
{
    return vm.count("quiet");
}

std::string CliBase::proxy() const
{
    if (vm.count("proxy"))
        {
            return vm["proxy"].as<std::string>();
        }

    const char* x509_user_proxy = getenv("X509_USER_PROXY");
    if (x509_user_proxy)
        {
            return x509_user_proxy;
        }

    std::ostringstream proxy_path;
    proxy_path << "/tmp/x509up_u" << geteuid();
    return proxy_path.str();
}


std::string CliBase::getService() const
{
    return endpoint;
}

std::string CliBase::discoverService() const
{
    return std::string();
}

std::string CliBase::getCliVersion() const
{
    return VERSION;
}
