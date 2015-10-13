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

#ifndef LISTVOMANAGER_H_
#define LISTVOMANAGER_H_

#include "CliBase.h"
#include <string>

namespace fts3
{
namespace cli
{

/**
 * The command line utility for specyfying VO name
 *
 * The class provides:
 *      - voname, which is a positional parameter if the constructor
 *          parameter was true, otherwise its an ordinary parameter
 *          (o option is also available in that case), allows for
 *          specifying the VO name
  */
class VoNameCli: virtual public CliBase
{
public:

    /**
     * Default Constructor.
     *
     * @param pos - if true VONAME is market as both: hidden and positional, otherwise it is a tool specific option
     *
     */
    VoNameCli(bool pos = true);

    /**
     * Destructor.
     */
    virtual ~VoNameCli();

    /**
     * Validates command line options
     * 1. Checks the endpoint
     * 2. If -h or -V option were used respective informations are printed
     * 3. GSoapContexAdapter is created, and info about server requested
     * 4. Additional check regarding server are performed
     * 5. If verbal additional info is printed
     *
     * @return GSoapContexAdapter instance, or null if all activities
     *              requested using program options have been done.
     */
    virtual void validate();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool) const;

    /**
     * Gets the VO name.
     *
     * @return VO name if it was specified, empty string otherwise
     */
    std::string getVoName();

private:

    bool pos;
};

}
}

#endif /* LISTVOMANAGER_H_ */
