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

/** \file configsetclioptions.h ConfigSetCliOptions class interface. */

#pragma once

#include "cli_dev.h"

#include <string>
#include <vector>

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/**
 * @brief ConfigSetCliOptions ... short description ...
 * @author Zsolt Molnar <Zsolt.Molnar@cern.ch>
 * @date 2012-03-22
 * ... description ...
 */
class ConfigSetCliOptions
{
    typedef std::vector<std::string> ConfigDataStoreType;

public:
    /** Default constructor */
    ConfigSetCliOptions(int argc, char** argv);

    /* ----------------------------------------------------------------------- */

    /** Destructor */
    virtual ~ConfigSetCliOptions()
    {
        // EMPTY
    }

    /* ----------------------------------------------------------------------- */

    bool isWriteHelp() const;

    /* ----------------------------------------------------------------------- */

    ConfigDataStoreType getData() const;

protected:

    /* ----------------------------------------------------------------------- */

    /** Copy constructor
     * @param other Reference on object to copy.
     */
    ConfigSetCliOptions(const ConfigSetCliOptions& other);

    /* ----------------------------------------------------------------------- */

    /** Assignment operator
     * @param other Reference on object to copy.
     * @return Reference on initialised object.
     */
    ConfigSetCliOptions& operator=(const ConfigSetCliOptions& other);

    /* ----------------------------------------------------------------------- */

    bool _isHelp;
    ConfigDataStoreType _configData;
};

FTS3_CLI_NAMESPACE_END

