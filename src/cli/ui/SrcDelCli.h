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

#ifndef SRCDESTCLI_H_
#define SRCDESTCLI_H_

#include "DelegationCli.h"
#include "TransferCliBase.h"
#include "SrcDestCli.h"
#include "TransferCliBase.h"

#include <vector>

namespace fts3
{
namespace cli
{

/**
 * SrcDestCli provides the CLI for specifying source and destination SE
 *
 * In addition to the functionalities inherited from CliBase, the class provides:
 *      - source (positional)
 *      - destination (positional)
 */
class SrcDelCli : public DelegationCli
{

public:
    /**
     * Default constructor.
     *
     * Creates FileName command line options
     * @return FileName string if it was given as CLI option, or an empty string if not
     */
    SrcDelCli();

    /*
     *  Destructor
     */
    ~SrcDelCli();

    /*
     *  Gets the file name (string) for the job.
     */
    std::vector<std::string> getFileName();

    /**
      * Validates command line options
      * 1. Checks the endpoint
      * 2. If -h or -V option were used respective informations are printed
      * 3. GSoapContexAdapter is created, and info about server requested
      * 4. Additional check regarding server are performed
      * 5. If verbal additional info is printed
      * @return GSoapContexAdapter instance, or null if all activities
      * requested using program options have been done.
      */
    void validate(bool init = true);

    ///
    static void validateFileName(std::string const & url);

    /**
     * the name of the file containing bulk-job description
     */
    std::string bulk_file;

    /**
     *  all the url's (files for deletion) which included in the file
     */
    std::vector<std::string> allFilenames;
};


}
}

#endif /* SRCDESTCLI_H_ */
