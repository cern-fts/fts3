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
