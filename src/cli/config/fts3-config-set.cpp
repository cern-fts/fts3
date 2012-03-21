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

/** \file fts3-config-set Implementation of fts3-config-set command. */

#include "cli_dev.h"
#include "fts3_config_set.h"

#include "common/logger.h"
#include "common/error.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace FTS3_COMMON_NAMESPACE;

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

Fts3ConfigSet::Fts3ConfigSet()
{
    // EMPTY
}

/* -------------------------------------------------------------------------- */

Fts3ConfigSet::~Fts3ConfigSet()
{
    // EMPTY
}

/* -------------------------------------------------------------------------- */

void Fts3ConfigSet::Execute
(
    int ,
    char**
)
{

}

/* -------------------------------------------------------------------------- */
#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Test if base constructors have been called */
BOOST_FIXTURE_TEST_CASE (TEST_NAME, Fts3ConfigSet)
{

}

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_CLI_NAMESPACE_END

/* -------------------------------------------------------------------------- */

int main (int argc, char** argv)
{
    int ret = 0;

    try
    {
        FTS3_CLI_NAMESPACE::Fts3ConfigSet::Execute(argc, argv);
    }
    catch(...)
    {
        FTS3_COMMON_LOGGER_NEWLOG (ERR) << "Unknown exception..." << commit;
        ret = -1;
    }

    return ret;
}

