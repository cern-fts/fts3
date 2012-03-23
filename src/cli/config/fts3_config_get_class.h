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

/** \file fts3_config_get.h Fts3ConfigGet class interface. */

#pragma once

#include "cli_dev.h"

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/**
 * @brief Fts3ConfigGet ... short description ...
 * @author Zsolt Molnar <Zsolt.Molnar@cern.ch>
 * @date 2012-03-21
 * ... description ...
 */
class Fts3ConfigGet
{
public:

    /** Destructor */
    virtual ~Fts3ConfigGet();

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
    Fts3ConfigGet();

    /* ----------------------------------------------------------------------- */

    /** Copy constructor
     * @param other Reference on object to copy.
     */
    Fts3ConfigGet(const Fts3ConfigGet& other);

    /* ----------------------------------------------------------------------- */

    /** Assignment operator
     * @param other Reference on object to copy.
     * @return Reference on initialised object.
     */
    Fts3ConfigGet& operator=(const Fts3ConfigGet& other);

    /* ----------------------------------------------------------------------- */
};

FTS3_CLI_NAMESPACE_END

