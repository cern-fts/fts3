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
#include <bits/local_lim.h>
#include <boost/filesystem/operations.hpp>
#include <common/Exceptions.h>
#include "version.h"

#include "exception/bad_option.h"


using namespace fts3::cli;

const std::string CliBase::error = "error";
const std::string CliBase::result = "result";
const std::string CliBase::parameter_error = "parameter_error";

CliBase::CliBase() : visible("Allowed options")
{

    // add basic command line options
    basic.add_options()
        ("help,h", "Print this help text and exit.")
        ("quiet,q", "Quiet operation.")
        ("verbose,v", "Be more verbose.")
        ("service,s", po::value<std::string>(), "Use the transfer service at the specified URL.")
        ("proxy", po::value<std::string>(), "Path to the proxy certificate (e.g. /tmp/x509up_u500).")
        ("insecure", "Disable the verification of the peer certificate.")
        ("version,V", "Print the version number and exit.");

    version = getCliVersion();
    interface = version;
}

void CliBase::parse(int ac, char *av[])
{

    // set the output parameters (verbose and json) before the actual parsing happens
    for (int i = 0; i < ac; i++) {
        std::string str(av[i]);
        if (str == "-v") {
            MsgPrinter::instance().setVerbose(true);
        }
        else if (str == "-j") {
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
}

void CliBase::validate()
{
    // check whether the -s option has been used
    const char *fts3_env;

    if (vm.count("service")) {
        endpoint = vm["service"].as<std::string>();
        // check if the endpoint has the right prefix
        if (endpoint.find("http") != 0 && endpoint.find("https") != 0 && endpoint.find("httpd") != 0) {
            std::string msg = "wrong endpoint format ('" + endpoint + "')";
            throw bad_option("service", msg);
        }
    }
    else if ((fts3_env = getenv("FTS3_ENDPOINT")) != NULL) {
        endpoint = fts3_env;
    }
    else if (access("/etc/fts3/fts3config", F_OK) == 0) {
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, sizeof(hostname));
        endpoint = std::string("https://") + hostname + ":8446";
    }
    else {
        throw bad_option("service", "Missing --service option");
    }

    // if endpoint could not be determined, we cannot do anything
    if (endpoint.empty()) {
        throw bad_option("service", "failed to determine the endpoint");
    }
}

void CliBase::printCliDetails() const
{
    MsgPrinter::instance().print_info("# Client version", "client_version", version);
    MsgPrinter::instance().print_info("# Client interface version", "client_interface", interface);
}

void CliBase::printApiDetails(ServiceAdapter &ctx) const
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
    if (vm.count("help")) {

        // remove the path to the executive
        std::string basename(toolname);
        size_t pos = basename.find_last_of('/');
        if (pos != std::string::npos) {
            basename = basename.substr(pos + 1);
        }
        // print the usage guigelines
        std::cout << std::endl << getUsageString(basename) << std::endl << std::endl;
        // print the available options
        std::cout << visible << std::endl;
        return true;
    }

    // check whether the -V option was used
    if (vm.count("version")) {
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

bool CliBase::isInsecure() const
{
    return vm.count("insecure");
}

bool CliBase::getProxyPath(CertKeyPair &pair) const
{
    std::string proxyPath;

    // Explicit options take precedence
    if (vm.count("proxy")) {
        proxyPath = vm["proxy"].as<std::string>();
    }
    else if (const char *x509_user_proxy = getenv("X509_USER_PROXY")) {
        proxyPath = x509_user_proxy;
    }

    if (!proxyPath.empty()) {
        pair.cert = pair.key = boost::filesystem::canonical(proxyPath).string();
        return true;
    }

    // Try default proxy location otherwise
    std::ostringstream proxyPathStr;
    proxyPathStr << "/tmp/x509up_u" << geteuid();
    proxyPath = proxyPathStr.str();

    if (access(proxyPath.c_str(), R_OK) == 0) {
        pair.cert = pair.key = boost::filesystem::canonical(proxyPath).string();
        return true;
    }
    return false;
}


bool CliBase::getUserCertAndKey(CertKeyPair &pair) const
{
    const char *ucert = getenv("X509_USER_CERT");
    const char *ukey = getenv("X509_USER_KEY");
    if (ucert && ukey) {
        pair.cert = boost::filesystem::canonical(ucert).string();
        pair.key = boost::filesystem::canonical(ukey).string();
        return true;
    }
    return false;
}


bool CliBase::getHostCertAndKey(CertKeyPair &pair) const
{
    static const char *hcert = "/etc/grid-security/hostcert.pem";
    static const char *hkey = "/etc/grid-security/hostkey.pem";

    if (geteuid() == 0 && access(hcert, R_OK) == 0 && access(hkey, R_OK) == 0) {
        pair.cert = boost::filesystem::canonical(hcert).string();
        pair.key = boost::filesystem::canonical(hkey).string();
        return true;
    }
    return false;
}


CertKeyPair CliBase::getCertAndKeyPair(void) const
{
    CertKeyPair pair;
    std::string proxyPath;

    try {
        getProxyPath(pair) || getUserCertAndKey(pair) || getHostCertAndKey(pair);
        MsgPrinter::instance().print_info("# User certificate", "user_cert", pair.cert);
        MsgPrinter::instance().print_info("# User key", "user_key", pair.key);
    }
    catch (const boost::filesystem::filesystem_error &e) {
        std::ostringstream errmsg;
        errmsg << "Failed to set the proxy: " << e.code().message() << ": " << e.path1();
        throw fts3::common::UserError(errmsg.str());
    }

    return pair;
}


std::string CliBase::getService() const
{
    return endpoint;
}


std::string CliBase::getCliVersion() const
{
    return VERSION;
}
