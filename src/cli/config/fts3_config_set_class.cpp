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

FTS3_CLI_NAMESPACE_START

/* -------------------------------------------------------------------------- */
struct Test_DummyParser
{
    typedef int RawDataType;
    typedef int ConfigDataType;

    static ConfigDataType Parse (const RawDataType&)
    {
        return 0;
    }
};

/* -------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST

struct TestCliOptions_Base
{
    TestCliOptions_Base()
    {
        HelpWritten = false;
    }

    /* ---------------------------------------------------------------------- */

    Test_DummyParser::RawDataType getData() const
    {
        return Test_DummyParser::RawDataType ();
    }

    /* ---------------------------------------------------------------------- */

    void writeHelp()
    {
        HelpWritten = true;
    }

    static bool HelpWritten;
};

bool TestCliOptions_Base::HelpWritten = false;

/* -------------------------------------------------------------------------- */

struct TestCliOptions_Help : public TestCliOptions_Base
{
    TestCliOptions_Help(int, char**)
        : TestCliOptions_Base()
    {}

    /* ---------------------------------------------------------------------- */

    bool isWriteHelp()
    {
        return true;
    }
};

/* -------------------------------------------------------------------------- */

struct TestCliOptions_NotHelp : public TestCliOptions_Base
{
    TestCliOptions_NotHelp(int, char**)
        : TestCliOptions_Base()
    {}

    /* ---------------------------------------------------------------------- */

    bool isWriteHelp()
    {
        return false;
    }
};

/* -------------------------------------------------------------------------- */

struct TestServiceCaller
{
    TestServiceCaller(const Test_DummyParser::ConfigDataType& )
    {
        IsCalled = false;
    }

    void call()
    {
        IsCalled = true;
    }

    static bool IsCalled;
};

bool TestServiceCaller::IsCalled = false;

/* -------------------------------------------------------------------------- */

struct TestTraits_Help
{
    typedef Test_DummyParser Parser;
    typedef TestCliOptions_Help CliOptions;
    typedef TestServiceCaller ServiceCaller;
};

/* -------------------------------------------------------------------------- */

typedef Fts3ConfigSet<TestTraits_Help> Test_Help;

/** Test if help "displayed" */
BOOST_FIXTURE_TEST_CASE (Fts3ConfigSet_HelpDisplayed, Test_Help)
{
    Execute(0, NULL);
    BOOST_CHECK (TestCliOptions_Help::HelpWritten);
}

/* -------------------------------------------------------------------------- */

struct TestTraits_NotHelp
{
    typedef Test_DummyParser Parser;
    typedef TestCliOptions_NotHelp CliOptions;
    typedef TestServiceCaller ServiceCaller;
};

/* -------------------------------------------------------------------------- */

typedef Fts3ConfigSet<TestTraits_NotHelp> Test_NotHelp;

/** Help should not been displayed, service caller must be called with the
 * proper parameters. */
BOOST_FIXTURE_TEST_CASE (Fts3ConfigSet_HelpNotDisplayed, Test_NotHelp)
{
    Execute(0, NULL);
    BOOST_CHECK (!TestCliOptions_Help::HelpWritten);
    BOOST_CHECK (TestServiceCaller::IsCalled);
}

#endif // FTS3_COMPILE_WITH_UNITTESTS

FTS3_CLI_NAMESPACE_END

