/*
 * Copyright (c) CERN 2013-2015
 *
 * Copyright (c) Members of the EMI Collaboration. 2010-2013
 *  See  http://www.eu-emi.eu/partners for details on the copyright
 *  holders.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <boost/test/unit_test_suite.hpp>
#include <boost/test/test_tools.hpp>

#include "cli/ui/CliBase.h"

using fts3::cli::CliBase;


/*
 * Dummy class that inherits after abstract CliBase
 * and implements its pure virtual method so the class
 * can be instantiated and tested
 */
class CliBaseTester : public CliBase
{

public:
    CliBaseTester() {}

    // implement the pure vitual method
    std::string getUsageString(std::string tool) const
    {
        return tool;
    };
};


BOOST_AUTO_TEST_SUITE(cli)
BOOST_AUTO_TEST_SUITE(CliBaseTest)


BOOST_AUTO_TEST_CASE (CliBaseShortOptions)
{
    std::vector<const char*> argv {
        "prog_name",
        "-h",
        "-q",
        "-v",
        "-s",
        "https://fts3-server:8080",
        "-V"
    };

    CliBaseTester cli;
    cli.parse(argv.size(), (char**)argv.data());

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(cli.printHelp());
    BOOST_CHECK(cli.isQuiet());
    BOOST_CHECK(cli.isVerbose());
    // the endpoint shouldn't be empty since it's starting with http
    //BOOST_CHECK(!cli.getService().empty());
}


BOOST_AUTO_TEST_CASE (CliBaseLongOptions)
{
    // has to be const otherwise is deprecated
    std::vector<const char*> argv {
        "prog_name",
        "--help",
        "--quiet",
        "--verbose",
        "--service",
        "https://fts3-server:8080",
        "--version"
    };

    CliBaseTester cli;
    cli.parse(argv.size(), (char**)argv.data());

    // all 5 parameters should be available in vm variable
    BOOST_CHECK(cli.printHelp());
    BOOST_CHECK(cli.isQuiet());
    BOOST_CHECK(cli.isVerbose());
    // the endpoint should be empty since it's not starting with http, https, httpd
    //BOOST_CHECK(!cli.getService().empty());
}


BOOST_AUTO_TEST_CASE (CliBaseExplicitProxyCmdLine)
{
    // has to be const otherwise is deprecated
    std::vector<const char*> argv {
        "prog_name",
        "--service",
        "https://fts3-server:8080",
        "--proxy", "/dev/null"
    };

    CliBaseTester cli;
    cli.parse(argv.size(), (char**)argv.data());

    fts3::cli::CertKeyPair certkey = cli.getCertAndKeyPair();

    BOOST_CHECK_EQUAL("/dev/null", certkey.cert);
    BOOST_CHECK_EQUAL(certkey.cert, certkey.key);
}


BOOST_AUTO_TEST_CASE (CliBaseExplicitProxyEnv)
{
    // has to be const otherwise is deprecated
    std::vector<const char*> argv {
        "prog_name",
        "--service",
        "https://fts3-server:8080",
    };

    CliBaseTester cli;
    cli.parse(argv.size(), (char**)argv.data());

    setenv("X509_USER_PROXY", "/dev/null", 1);

    fts3::cli::CertKeyPair certkey = cli.getCertAndKeyPair();

    unsetenv("X509_USER_PROXY");

    BOOST_CHECK_EQUAL("/dev/null", certkey.cert);
    BOOST_CHECK_EQUAL(certkey.cert, certkey.key);
}


BOOST_AUTO_TEST_CASE (CliBaseExplicitCertAndKey)
{
    // has to be const otherwise is deprecated
    std::vector<const char*> argv {
        "prog_name",
        "--service",
        "https://fts3-server:8080",
    };

    CliBaseTester cli;
    cli.parse(argv.size(), (char**)argv.data());

    unsetenv("X509_USER_PROXY");
    setenv("X509_USER_CERT", "/dev/zero", 1);
    setenv("X509_USER_KEY", "/dev/urandom", 1);

    fts3::cli::CertKeyPair certkey = cli.getCertAndKeyPair();

    unsetenv("X509_USER_CERT");
    unsetenv("X509_USER_KEY");

    BOOST_CHECK_EQUAL("/dev/zero", certkey.cert);
    BOOST_CHECK_EQUAL("/dev/urandom", certkey.key);
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
