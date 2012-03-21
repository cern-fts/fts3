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

/** \file fts3_config_set.h Fts3ConfigSet class interface. */

#pragma once

#include "cli_dev.h"

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/**
 * @brief Fts3ConfigSet ... short description ...
 * @author Zsolt Molnar <Zsolt.Molnar@cern.ch>
 * @date 2012-03-21
 * ... description ...
 */
class Fts3ConfigSet
{
public:

    /** Destructor */
    virtual ~Fts3ConfigSet();

    /* ----------------------------------------------------------------------- */

    /** Execute the command */
    static void Execute
    (
        int argc, /**< Command line options - from main */
        char** argv /**< Command line options - from main */
    );

protected:

    /* ----------------------------------------------------------------------- */

    /** Default constructor */
    Fts3ConfigSet();

    /* ----------------------------------------------------------------------- */

    /** Copy constructor
     * @param other Reference on object to copy.
     */
    Fts3ConfigSet(const Fts3ConfigSet& other);

    /* ----------------------------------------------------------------------- */

    /** Assignment operator
     * @param other Reference on object to copy.
     * @return Reference on initialised object.
     */
    Fts3ConfigSet& operator=(const Fts3ConfigSet& other);

    /* ----------------------------------------------------------------------- */
};

FTS3_CLI_NAMESPACE_END

