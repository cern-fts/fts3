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

#ifndef CLIBASE_H_
#define CLIBASE_H_

#include "ServiceAdapter.h"
#include "MsgPrinter.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include <fstream>
#include <vector>


namespace fts3
{
namespace cli
{

namespace po = boost::program_options;

/**
 * CliBase is a base class for developing FTS3 command line tools.
 *
 * The class provides following functionalities:
 *      - help (-h)
 *      - quite (-q) and verbose (-v)
 *      - version (-V)
 *      - service (-s)
 *
 *  The CliBase is an abstract class, each class that inherits after CliBase
 *  has to implement 'getUsageString()', which returns an instruction on how
 *  to use the given command line tool.
 */
class CliBase
{

public:

    ///
    static const std::string error;
    ///
    static const std::string result;
    ///
    static const std::string parameter_error;

    /**
     * Default constructor.
     *
     * Initialises service discovery parameters. Moreover creates the basic
     * command line options, and marks them as visible.
     */
    CliBase();

    /**
     * Destructor
     */
    virtual ~CliBase() {}

    /**
     * Initializes the object with command line options.
     *
     * Parses the command line options that were passed to the command line tool.
     * In addition check whether the format of the FTS3 service is correct. If
     * The FTS3 service is not specified the object tries to discover it.
     *
     * @param ac - argument count
     * @param av - argument array
     */
    virtual void parse(int ac, char* av[]);

    /**
     * Validates command line options
     * 1. Checks the endpoint
     * 2. If -h or -V option were used respective informations are printed
     * 3. Additional check regarding server are performed
     */
    virtual void validate();

    /**
     * If verbose additional info is printed
     */
    void printApiDetails(ServiceAdapter & ctx) const;

    /**
     * Prints help message if the -h option has been used.
     *
     * @param tool - the name of the executive that has been called (in most cases argv[0])
     *
     * @return true if the help message has been printed
     */
    virtual bool printHelp() const;

    /**
     * Checks whether the -v option was used.
     *
     * @return true if -v option has been used
     */
    bool isVerbose() const;

    /**
     * Checks whether the -q option was used.
     *
     * @return true if -q option has been used
     */
    bool isQuiet() const;

    /**
     * @return the path to the proxy certificate
     */
    std::string proxy() const;

    /**
     * Gets the FTS3 service string
     *
     * @return FTS3 service string
     */
    std::string getService() const;

    /**
     * Pure virtual method, it aim is to give the instruction how to use the command line tool.
     *
     * @param tool - name of the fts3 tool that is using this utility (e.g. 'fts3-transfer-submit')
     *
     * @return implementing class should return a string with instruction on how to use the tool
     */
    virtual std::string getUsageString(std::string tool) const;

protected:

    /**
     * If verbose additional info is printed
     */
    void printCliDetails() const;

    /**
     * check if it's possible to use fts3 server config file to discover the endpoint
     *
     * @return true
     */
    virtual bool useSrvConfig()
    {
        return true;
    }

    /**
     * Discovers the FTS3 service (if the -s option has not been used).
     *
     * Uses ServiceDiscoveryIfce to find a FTS3 service.
     *
     * @return FTS3 service string
     */
    std::string discoverService() const;

    /**
     * a map containing parsed options
     */
    po::variables_map vm;

    /**
     * basic command line options provided by CliBase
     */
    po::options_description basic;

    /**
     * command line options that are printed if -h option has been used
     */
    po::options_description visible;

    /**
     * all command line options, inheriting class should add its options to 'cli_option'
     */
    po::options_description all;

    /**
     * command line parameters that are passed without any switch option e.g. -p
     */
    po::positional_options_description p;

    /**
     * command line options specific for fts3-transfer-status
     */
    po::options_description specific;

    /**
     * hidden command line options (not printed in help)
     */
    po::options_description hidden;

    /**
     * command line option specific for a command
     */
    po::options_description command_specific;

    /**
     * the FTS3 service endpoint
     */
    std::string endpoint;

    /**
     * the name of the utility
     */
    std::string toolname;

private:

    /**
     *
     */
    std::string getCliVersion() const;

    std::string version;
    std::string interface;

    ///@{
    /**
     * string values used for discovering the FTS3 service
     */
    std::string FTS3_CA_SD_TYPE;
    std::string FTS3_SD_ENV;
    std::string FTS3_SD_TYPE;
    std::string FTS3_IFC_VERSION;
    std::string FTS3_INTERFACE_VERSION;
    ///@}
};

}
}

#endif /* CLIBASE_H_ */
