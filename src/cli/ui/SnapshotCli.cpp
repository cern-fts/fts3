/*
 * SnapshotCli.cpp
 *
 *  Created on: 10 Mar 2014
 *      Author: simonm
 */

#include "SnapshotCli.h"

namespace fts3
{

namespace cli
{

SnapshotCli::SnapshotCli() : SrcDestCli(false)
{

    specific.add_options()
    ("vo", value<std::string>(), "Specify VO name.")
    ;
}

SnapshotCli::~SnapshotCli()
{

}

std::string SnapshotCli::getVo()
{
    // check if vo was passed via command line options
    if (vm.count("vo"))
        {
            return vm["vo"].as<std::string>();
        }
    return "";
}

} /* namespace cli */
} /* namespace fts3 */
