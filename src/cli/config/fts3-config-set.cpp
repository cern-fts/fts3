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
#include "fts3_config_set_class.h"

#include "common/logger.h"
#include "common/error.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace FTS3_COMMON_NAMESPACE;

/* -------------------------------------------------------------------------- */

int main (int , char** )
{
    int ret = 0;
    //ret = FTS3_CLI_NAMESPACE::Fts3ConfigSet::Execute(argc, argv);

    return ret;
}

