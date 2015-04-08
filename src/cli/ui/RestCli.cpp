/*
 * RestCli.cpp
 *
 *  Created on: Feb 6, 2014
 *      Author: simonm
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
    ("proxy", po::value<std::string>(),  "Path to the proxy certificate (e.g. /tmp/x509up_u500).")
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

std::string RestCli::proxy() const
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
