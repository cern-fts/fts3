/*
 * RestCli.cpp
 *
 *  Created on: Feb 6, 2014
 *      Author: simonm
 */

#include "RestCli.h"

using namespace fts3::cli;

RestCli::RestCli()
{
    // add fts3-transfer-status specific options
    specific.add_options()
    ("rest", "Use the RESTful interface.")
    ;
}

RestCli::~RestCli()
{

}

bool RestCli::rest()
{
    // return true if rest is in the map
    return vm.count("rest");
}
