/* Copyright @ Members of the EMI Collaboration, 2010.
See www.eu-emi.eu for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/** \file parser.h Parser class interface. */

#pragma once

#include "cli_dev.h"

#include <string>
#include <map>
#include <vector>

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/**
 * @brief Parser ... short description ...
 * @author Zsolt Molnar <Zsolt.Molnar@cern.ch>
 * @date 2012-03-23
 * ... description ...
 */
class Parser
{
protected:

     typedef std::pair<std::string, std::string> TokenizerResultType;

public:

    typedef std::map<std::string, std::string> ConfigDataType;
    typedef std::vector<std::string> RawDataType;

    /** Default constructor */
    Parser()
    {
        // EMPTY
    }

    /* ----------------------------------------------------------------------- */

    /** Destructor */
    virtual ~Parser()
    {
        // EMPTY
    }

    /* ----------------------------------------------------------------------- */

    static ConfigDataType Parse (const RawDataType& rawData);


protected:

    /* ----------------------------------------------------------------------- */

    static TokenizerResultType Tokenize (const std::string& line);

    /* ----------------------------------------------------------------------- */

    /** Copy constructor
     * @param other Reference on object to copy.
     */
    Parser(const Parser& other);

    /* ----------------------------------------------------------------------- */

    /** Assignment operator
     * @param other Reference on object to copy.
     * @return Reference on initialised object.
     */
    Parser& operator=(const Parser& other);

    /* ----------------------------------------------------------------------- */
};

FTS3_CLI_NAMESPACE_END

