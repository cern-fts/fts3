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

/** \file configsetclioptions.cpp Implementation of ConfigSetCliOptions class. */

#include "cli_dev.h"
#include "config_set_cli_options.h"

#include "common/logger.h"
#include "common/error.h"

#include <boost/program_options.hpp>

#ifdef FTS3_COMPILE_WITH_UNITTEST
    #include "unittest/testsuite.h"
#endif // FTS3_COMPILE_WITH_UNITTESTS

using namespace FTS3_COMMON_NAMESPACE;

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */

ConfigSetCliOptions::ConfigSetCliOptions
(
    int argc,
    char** argv
)
    : _isHelp(false)
{
    po::options_description desc = _defineOptions();

    po::store(po::parse_command_line(argc, argv, desc), _vm);

    if (_vm.count("help"))
    {
        _isHelp = true;
        return;
    }

    po::notify(_vm);
}

/* -------------------------------------------------------------------------- */

bool ConfigSetCliOptions::isWriteHelp() const
{
    return _isHelp;
}

/* -------------------------------------------------------------------------- */

ConfigSetCliOptions::ConfigDataStoreType ConfigSetCliOptions::getData() const
{
    return _configData;
}

/* -------------------------------------------------------------------------- */

po::options_description ConfigSetCliOptions::_defineOptions()
{
    po::options_description generic("FTS3 server config options. See: https://svnweb.cern.ch/trac/fts3/wiki/Configuration");

    generic.add_options()
        ("help,h", "Display this help page")

        (
            "values,v",
            po::value< std::vector<std::string> >( &_configValues )->required(),
            "Configuration parameters. List of strings in \"key=value\" format."
        );

    return generic;
}

/* -------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

/** Test if base constructors have been called */
BOOST_FIXTURE_TEST_CASE (TEST_NAME, ConfigSetCliOptions)
{

    static const int argc = 3;
    char *argv[argc];
    argv[0] = const_cast<char*> ("executable");
    argv[1] = const_cast<char*> ("--configfile=/dev/null");
    argv[2] = const_cast<char*> ("--Port=7823682");
    (*this)(argc, argv);
    BOOST_CHECK_EQUAL (_vars["Port"], std::string("7823682"));
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* -------------------------------------------------------------------------- */

FTS3_CLI_NAMESPACE_END

