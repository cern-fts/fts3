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

/** \file fts3configcommon.h Fts3ConfigCommon class interface. */

#pragma once

#include "cli_dev.h"

#include "common/logger.h"
#include "common/error.h"

#include <iostream>

using namespace FTS3_COMMON_NAMESPACE;

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

/**
 * @brief Fts3ConfigCommon ... short description ...
 * @author Zsolt Molnar <Zsolt.Molnar@cern.ch>
 * @date 2012-03-23
 * ... description ...
 */
template <class BASE>
class Fts3ConfigCommon
{
public:
    /** Default constructor */
    Fts3ConfigCommon()
    {
        // EMPTY
    }

    /* ----------------------------------------------------------------------- */

    /** Destructor */
    virtual ~Fts3ConfigCommon()
    {
        // EMPTY
    }

    /* ----------------------------------------------------------------------- */

    static int Execute(int argc, char** argv)
    {
        int res = -1;

        try
        {
            BASE::ExecuteCommand(argc, argv);
            res = 0;
        }
        catch (Err& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "FTS3 client error " << commit;
        }
        catch (std::exception& e)
        {
            FTS3_COMMON_LOGGER_NEWLOG(ERR) << "C++ error in FTS3 client: " << e.what() << commit;
        }
        catch (...)
        {
            std::cerr << "Unknown exception " << std::endl;
        }

        return res;
    }

protected:

    /* ----------------------------------------------------------------------- */

    /** Copy constructor
     * @param other Reference on object to copy.
     */
    Fts3ConfigCommon(const Fts3ConfigCommon& other);

    /* ----------------------------------------------------------------------- */

    /** Assignment operator
     * @param other Reference on object to copy.
     * @return Reference on initialised object.
     */
    Fts3ConfigCommon& operator=(const Fts3ConfigCommon& other);

    /* ----------------------------------------------------------------------- */
};

FTS3_CLI_NAMESPACE_END

