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

#ifndef JOBIDCLI_H_
#define JOBIDCLI_H_

#include "CliBase.h"
#include "TransferCliBase.h"
#include <vector>
#include <string>

namespace fts3
{
namespace cli
{

/**
 * JobIDCli is the command line utility used for retreiving job IDs.
 *
 * In addition to the inherited functionalities from CliBase the SubmitTransferCli class provides:
 * 		- job ID (--jobid), positional parameter (passed without any switch option)
 *
 * @see CliBase
 */
class JobIdCli: public TransferCliBase
{
public:

    /**
     * Default constructor.
     *
     * Creates the command line interface for retrieving job IDs. Job ID is
     * market as both: hidden and positional
     */
    JobIdCli();

    /**
     * Destructor.
     */
    virtual ~JobIdCli();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     * Gets a vector with job IDs.
     *
     * @return if job IDs were given as command line parameters a
     * 			vector containing job IDs otherwise an empty vector
     */
    std::vector<std::string> getJobIds();
};

}
}

#endif /* JOBIDCLI_H_ */
