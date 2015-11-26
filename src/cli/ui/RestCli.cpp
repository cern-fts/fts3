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
    ("capath", po::value<std::string>(),  "Path to the GRID security certificates (e.g. /etc/grid-security/certificates).")
    ;

    // hidden option. Now replaced with non-hidden option with opposite sense.
    hidden.add_options()
    ("rest", "Use the RESTful interface.")
    ;
}

RestCli::~RestCli()
{

}

std::string RestCli::capath() const
{
    if (vm.count("capath"))
    {
        return vm["capath"].as<std::string>();
    }

    // if there is no value specified RestContextAdapter will check the
    // environment or apply a default
    return std::string();
}
