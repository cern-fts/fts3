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
 * SubmitTransferCli.h
 *
 *  Created on: Feb 2, 2012
 *      Author: Michal Simon
 */

#ifndef SUBMITTRANSFERCLI_H_
#define SUBMITTRANSFERCLI_H_

#include "SrcDestCli.h"
#include "DelegationCli.h"
#include "File.h"

#include <vector>

#include <boost/optional.hpp>

namespace fts3
{
namespace cli
{

/**
 * SubmitTransferCli is the command line utility used for the fts3-transfer-submit tool.
 *
 * In addition to the inherited functionalities from CliBase the SubmitTransferCli class provides:
 * 	 	- blocking (-b)
 * 	 	- bulk-job file (-f)
 * 	 	- grid ftp parameters (-g)
 * 	 	- interval (-i)
 * 	 	- MyProxy server (-m)
 * 	 	- password (-p)
 * 	 	- ID (-I)
 * 	 	- expire (-e)
 * 	 	- overwrite (-o)
 * 	 	- destination token (-t)
 * 	 	- source token (-S)
 * 	 	- compare checksum (-K)
 * 	 	- pin lifetime (--copy-pin-lifetime)
 * 	 	- LAN connection (--lan-connection)
 * 	 	- fail if nearline (--fail-nearline)
 *
 * @see CliBase
 */
class SubmitTransferCli : public SrcDestCli, public DelegationCli
{

public:

    /**
     * Default constructor.
     *
     * Initializes string fields corresponding to FTS3 parameters. Moreover, creates
     * the transfer-submit specific command line options, checksum is also marked as
     * positional options, checksum is a hidden option (not printed in help),
     * other options are visible.
     */
    SubmitTransferCli();

    /**
     * Destructor
     */
    virtual ~SubmitTransferCli();

    /**
     * Initializes the object with command line options.
     *
     * In addition sets delegation flag.
     *
     * @param ac - argument count
     * @param av - argument array
     *
     * @see CliBase::initCli(int, char*)
     */
    virtual void parse(int ac, char* av[]);

    /**
     * Validates command line options
     * 1. Checks the endpoint
     * 2. If -h or -V option were used respective informations are printed
     * 3. GSoapContexAdapter is created, and info about server requested
     * 4. Additional check regarding server are performed
     * 5. If verbal additional info is printed
     *
     * @return GSoapContexAdapter instance, or null if all activities
     * 				requested using program options have been done.
     */
    void validate();

    /**
     * Creates job elements.
     *
     * The job elements are created based on used command line options
     * or based on the given bulk-job file. The created elements are
     * stored in an internal vector.
     *
     * @return true is the internal vector containing job elements is not empty
     */
    void createJobElements();

    /**
     * Performs standard checks.
     *
     *  	- decides on delegation (checks whether it is supported)
     *  	- collects user password if needed
     *  	- checks if the job is specified twice (using command line and bulk-job file)
     *  	- checks if used options are supported (e.g. checksum)
     */
    void performChecks();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     * Gets the password (if set using -p option or by performChecks()
     *
     * @return password string
     *
     * @see SubmitTransferCli::performChecks()
     */
    std::string getPassword();

    /**
     * Gets a vector containing string-string pairs.
     *
     * The parameters are set accordingly to the options used with the command line tool.
     *
     * @return a vector with key-value parameters
     */
    std::map<std::string, std::string> getParams();

    /**
     * Gets a vector containing job elements (files).
     *
     * The returned vector is created based on the internal vector created using 'createJobElements()'.
     *
     * @return vector of Files
     *
     * @see SubmitTransferCli::createJobElements()
     */
    std::vector<File> getFiles();

    /**
     * Gets job's metadata
     *
     * @return job's metadata if there were some, an uninitialized optional otherwise
     */
    boost::optional<std::string> getMetadata();

    /**
     * Gets the value of delegation flag.
     *
     * @return true if the proxy certificate should be delegated
     */
    bool useDelegation()
    {
        return delegate;
    }

    /**
     * Gets the value of checksum flag.
     *
     * @return true if checksum is used
     */
    bool useCheckSum()
    {
        return checksum;
    }

    /**
     * Check if the blocking mode is on.
     *
     * @return true if the -b option has been used
     */
    bool isBlocking();


    int recognizeParameter(std::string str);

    /**
     * gets the (bulk submission) file name
     */
    std::string getFileName();


protected:

    /**
     * check if it's possible to use fts3 server config file to discover the endpoint
     *
     * @return false
     */
    virtual bool useSrvConfig()
    {
        return false;
    }

private:

    /**
     * Ask user for password.
     *
     * @return user password
     */
    std::string askForPassword();

    static void parseMetadata(std::string const & metadata);

    /**
     * checks if the provided url is valid
     */
    static bool checkValidUrl(const std::string &uri);

    /**
     * the name of the file containing bulk-job description
     */
    std::string bulk_file;

    /**
     * user password
     */
    std::string password;

    /**
     * checksum flag, determines whether checksum is used
     */
    bool checksum;

    /**
     * delegate flag, determines whether the proxy certificate should be used
     */
    bool delegate;

    /**
     * transfer job elements (files)
     */
    std::vector<File> files;

};

}
}

#endif /* SUBMITTRANSFERCLI_H_ */
