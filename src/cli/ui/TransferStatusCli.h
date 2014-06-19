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
 */

/*
 * TransferStatusCli.h
 *
 *  Created on: Feb 13, 2012
 *      Author: simonm
 */

#ifndef TRANSFERSTATUSCLI_H_
#define TRANSFERSTATUSCLI_H_

#include "JobIdCli.h"
#include <vector>
#include <string>

using namespace std;

namespace fts3
{
namespace cli
{

/**
 * TransferStatusCli is the command line utility used for the fts3-transfer-status tool.
 *
 * In addition to the inherited functionalities from CliBase the SubmitTransferCli class provides:
 * 		- list (-l)
 * 		- job ID (--jobid), positional parameter (passed without any switch option)
 *
 * @see CliBase
 */
class TransferStatusCli: public JobIdCli
{
public:
    /**
     * Default constructor.
     *
     * Creates the transfer-status specific command line options. Job ID is
     * market as both: hidden and positional
     */
    TransferStatusCli();

    /**
     * Destructor.
     */
    virtual ~TransferStatusCli();

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
    bool validate();


    /**
     * Check if the list mode is on.
     *
     * @return true if the -l option has been used
     */
    bool list();

    /**
     * Check if querying the archive is on
     */
    bool queryArchived();

    /**
     * If true, the failed files should be dump to a file
     */
    bool dumpFailed();

    /**
     * If true, be more detailed
     */
    bool detailed();

    /**
     * true if p option was used, false otherwise
     */
    bool p();
};

}
}

#endif /* TRANSFERSTATUSCLI_H_ */
