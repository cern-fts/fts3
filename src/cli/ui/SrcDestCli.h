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
 * SrcDestCli.h
 *
 *  Created on: Aug 2, 2012
 *      Author: Michał Simon
 */

#ifndef SRCDESTCLI_H_
#define SRCDESTCLI_H_

#include "CliBase.h"


namespace fts3
{
namespace cli
{

/**
 * SrcDestCli provides the CLI for specifying source and destination SE
 *
 * In addition to the functionalities inherited from CliBase, the class provides:
 *  	- source (positional)
 *  	- destination (positional)
 */
class SrcDestCli : virtual public CliBase
{

public:

    /**
     * Default constructor.
     *
     * Creates source and destination command line options
     * (both are marked as positional options).
     */
    SrcDestCli(bool hide = false);

    /**
     * Destructor
     */
    virtual ~SrcDestCli();

    /**
     * Gets the source file name (string) for the job.
     *
     * @return source string if it was given as a CLI option, or an empty string if not
     */
    std::string getSource();

    /**
     * Gets the destination file name (string) for the job.
     *
     * @return destination string if it was given as a CLI option, or an empty string if not
     */
    std::string getDestination();
};

}
}

#endif /* SRCDESTCLI_H_ */
