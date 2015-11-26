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
 * DebugSetCli.h
 *
 *  Created on: Aug 2, 2012
 *      Author: Michał Simon
 */

#ifndef DEBUGSETCLI_H_
#define DEBUGSETCLI_H_

#include "RestCli.h"

#include <string>
#include <vector>

namespace fts3
{
namespace cli
{

/**
 *
 */
class DebugSetCli : public RestCli
{

public:

    /**
     * Default Constructor.
     *
     * Creates the debud-set specific command line option: on/off.
     */
    DebugSetCli();

    /**
     * Destructor.
     */
    virtual ~DebugSetCli();

    /**
     * Validates command line options.
     *	Checks that the debug mode was set correctly.
     *
     * @return GSoapContexAdapter instance, or null if all activities
     * 				requested using program options have been done.
     */
    virtual void validate();

    /**
     * Gives the instruction how to use the command line tool.
     *
     * @return a string with instruction on how to use the tool
     */
    std::string getUsageString(std::string tool);

    /**
     * Gets the debug level.
     *
     * @return 0 if debug mode is off, an integer specifying the debug level otherwise
     */
    unsigned getDebugLevel()
    {
        return level;
    }

    /**
     * Gets the source file name (string) for the job.
     *
     * @return source string if it was given as a CLI option, or an empty string if not
     */
    std::string getSource()
    {
        return source;
    }

    /**
     * Gets the destination file name (string) for the job.
     *
     * @return destination string if it was given as a CLI option, or an empty string if not
     */
    std::string getDestination()
    {
        return destination;
    }

private:

    /// debug level
    unsigned level;

    /// source
    std::string source;

    // destination
    std::string destination;
};

}
}

#endif /* DEBUGSETCLI_H_ */
