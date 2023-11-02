/**
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
#include <boost/filesystem.hpp>
#include <fstream>

#include "common/Exceptions.h"
#include "config/ServerConfigReader.h"

#include "version.h"


BOOST_AUTO_TEST_SUITE(config)
BOOST_AUTO_TEST_SUITE(ServerConfigReaderTestSuite)


BOOST_FIXTURE_TEST_CASE (functionOperator, fts3::config::ServerConfigReader)
{
    std::vector<const char*> args{
        "executable", "--configfile=/dev/null",
        "--Port=7823682", "--SiteName", "required"
    };

    (*this)(args.size(), (char**)args.data());
    BOOST_CHECK_EQUAL(_vars["Port"], std::string("7823682"));
    BOOST_CHECK_EQUAL(_vars["SiteName"], std::string("required"));
}

BOOST_FIXTURE_TEST_CASE (transferOperator, fts3::config::ServerConfigReader)
{
    std::vector<const char*> args{
            "executable",
            "--TransfersServiceAllocatorAlgorithm=MAXIMUM_FLOW",
            "--TransfersServiceSchedulingAlgorithm=DEFICIT",
            "--SiteName", "required", 
    };

    (*this)(args.size(), (char**)args.data());
    BOOST_CHECK_EQUAL(_vars["TransfersServiceAllocatorAlgorithm"], std::string("MAXIMUM_FLOW"));
    BOOST_CHECK_EQUAL(_vars["TransfersServiceSchedulingAlgorithm"], std::string("DEFICIT"));
}

// Test class for DbType options.
struct TestDbTypeServerConfigReader : public fts3::config::ServerConfigReader
{
    TestDbTypeServerConfigReader():
        argv{"executable", "--configfile=/dev/null", "--SiteName", "required"}
    {
    }

    void doTest()
    {
        (*this)(argv.size(), (char**)argv.data());
        BOOST_CHECK_EQUAL (_vars["DbType"], std::string("atyala"));
    }

    std::vector<const char*> argv;
};


BOOST_FIXTURE_TEST_CASE (passDbTypeShortForm, TestDbTypeServerConfigReader)
{
    argv.push_back("-d");
    argv.push_back("atyala");
    doTest();
}


BOOST_FIXTURE_TEST_CASE (passDbTypeLongForm, TestDbTypeServerConfigReader)
{
    argv.push_back("--DbType=atyala");
    doTest();
}


BOOST_FIXTURE_TEST_CASE (functionOperatorFromFile, fts3::config::ServerConfigReader)
{
    // Open a temporary file
    char filename[] = "/tmp/fts3tests.XXXXXX";
    int fd = mkstemp(filename);
    BOOST_CHECK_GT (fd, -1);
    close(fd);
    std::ofstream file(filename);

    // Write a temporary config file
    std::string f_intval = "32234";
    std::string f_str = "randomval";
    file << "Port=" << f_intval << std::endl;
    file << "IP=" << f_str << std::endl;
    file << "DbConnectString=" << f_str << std::endl;
    file << "DbUserName=" << f_str << std::endl;
    file << "DbPassword=" << f_str << std::endl;
    file << "TransferLogDirectory=" << f_str << std::endl;
    file << "ThreadNum=" << f_intval << std::endl;
    file << "TransfersServiceSchedulingAlgorithm=" << f_str << std::endl;
    file << "TransfersServiceAllocatorAlgorithm=" << f_str << std::endl;
    file << "TransfersServiceAllocatorLambda=" << f_intval << std::endl;
    file.close();

    // Read from the file
    std::string confpar = std::string("--configfile=") + filename;
    std::vector<const char*> argv{
        "executable", confpar.c_str(), "--SiteName", "required"
    };

    (*this)(argv.size(), (char**)argv.data());

    // Do the checks
    BOOST_CHECK_EQUAL (_vars["Port"], f_intval);
    BOOST_CHECK_EQUAL (_vars["ThreadNum"], f_intval);
    BOOST_CHECK_EQUAL (_vars["IP"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbConnectString"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbUserName"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbPassword"], f_str);
    BOOST_CHECK_EQUAL (_vars["TransferLogDirectory"], f_str);
    BOOST_CHECK_EQUAL (_vars["TransfersServiceSchedulingAlgorithm"], f_str);
    BOOST_CHECK_EQUAL (_vars["TransfersServiceAllocatorAlgorithm"], f_str);

    BOOST_CHECK_EQUAL (_vars["TransfersServiceAllocatorLambda"], f_intval);

    BOOST_CHECK_NO_THROW(boost::filesystem::remove(filename));
}


// Traits to test _readCommandLineOptions
struct ReadCommandLineOptionsTestTraits
{
    static void exit(const int)
    {
        exitCalled = true;
    }

    static void processVariables(fts3::config::ServerConfigReader&)
    {
        processVariablesCalled = true;
    }

    static void reset()
    {
        processVariablesCalled = false;
        exitCalled = false;
        strstream.str("");
    }

    static std::ostream& stream()
    {
        return strstream;
    }

    static bool processVariablesCalled;
    static bool exitCalled;
    static std::stringstream strstream;
};

bool ReadCommandLineOptionsTestTraits::processVariablesCalled;
bool ReadCommandLineOptionsTestTraits::exitCalled;
std::stringstream ReadCommandLineOptionsTestTraits::strstream;


struct TestServerConfigReader : public fts3::config::ServerConfigReader
{
    TestServerConfigReader()
    {
        argv[0] = const_cast<char*> ("executable");
        testDesc.add_options()("help,h", "Description");
        testDesc.add_options()("version", "Description");
        testDesc.add_options()("no-daemon,n", "Description");
        testDesc.add_options()("other", boost::program_options::value<std::string>(), "Description");
        testDesc.add_options()("intpar", boost::program_options::value<int>(), "Description");
    }

    void setupParameters(const std::string& aOption)
    {
        ReadCommandLineOptionsTestTraits::reset();
        argv[1] = const_cast<char*> (aOption.c_str());
    }

    // This test checks if:
    //    - --help option recognized
    //    - help message displayed
    //    - program exits
    void do_helpTest()
    {
        _readCommandLineOptions<ReadCommandLineOptionsTestTraits>(argc, argv, testDesc);
        std::string f_helpMessage("-h [ --help ]         Description");
        std::string displayedText = ReadCommandLineOptionsTestTraits::strstream.str();
        bool contained = (displayedText.find(f_helpMessage) != std::string::npos);
        BOOST_CHECK (contained);
    }

    // This test checks if:
    //    - version option displays FTS3 version string
    //    - program exits
    void do_versionTest()
    {
        _readCommandLineOptions<ReadCommandLineOptionsTestTraits>(argc, argv, testDesc);
        std::string f_versionMessage(VERSION);
        std::string displayedText = ReadCommandLineOptionsTestTraits::strstream.str();
        bool contained = (displayedText.find(f_versionMessage) != std::string::npos);
        BOOST_CHECK (contained);
    }

    // This test checks if:
    //    - Any other options than helo or version calls provessVariables
    //    - program does not exit
    void do_othersTest()
    {
        _readCommandLineOptions<ReadCommandLineOptionsTestTraits>(argc, argv, testDesc);
        BOOST_CHECK ( ! ReadCommandLineOptionsTestTraits::exitCalled);
        BOOST_CHECK ( ReadCommandLineOptionsTestTraits::processVariablesCalled);
    }

    // This test checks the effect of nodaemon flags.
    void do_noDaemonSpecifiedTest()
    {
        _readCommandLineOptions<ReadCommandLineOptionsTestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string("true"));
    }

    // This test checks the effect of nodaemon flags.
    void do_noDaemonNotSpecifiedTest()
    {
        _readCommandLineOptions<ReadCommandLineOptionsTestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string());
    }

protected:
    static const int argc = 2;
    char *argv[argc];
    boost::program_options::options_description testDesc;
};


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckHelpLongForm, TestServerConfigReader)
{
    // Test executing long help option
    setupParameters ("--help" );
    do_helpTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckHelpShortForm, TestServerConfigReader)
{
    // Test executing short help option
    setupParameters ("-h" );
    do_helpTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckVersion, TestServerConfigReader)
{
    // Test executing "version"
    setupParameters("--version");
    do_versionTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckPassValue, TestServerConfigReader)
{
    // Test executing "other" parameter. Test fixture requires parameter to
    // other!
    setupParameters("--other=value");
    do_othersTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckNoDaemonLongForm, TestServerConfigReader)
{
    setupParameters ("--no-daemon" );
    do_noDaemonSpecifiedTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckNoDaemonShortForm, TestServerConfigReader)
{
    setupParameters ("-n" );
    do_noDaemonSpecifiedTest();
}


BOOST_FIXTURE_TEST_CASE(readCommandLineOptionsCheckNoDaemonMissing, TestServerConfigReader)
{
    setupParameters ("--help" );
    do_noDaemonNotSpecifiedTest();
}


BOOST_FIXTURE_TEST_CASE(storeAString, TestServerConfigReader)
{
    setupParameters("--intpar=10");
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, testDesc), _vm);
    boost::program_options::notify(_vm);
    // Execute test - with existing parameter
    storeAsString ("intpar");
    // Do the checks
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string ("10"));
}


// Traits to test _readConfigFile
struct ReadConfigFileTestTraits
{
    static std::shared_ptr<std::istream> getStream (const std::string&)
    {
        std::shared_ptr<std::istream> ss(new std::stringstream("intpar=10"));
        return ss;
    }

    static void processVariables(fts3::config::ServerConfigReader& reader)
    {
        reader.storeAsString("intpar");
    }
};


BOOST_FIXTURE_TEST_CASE (readConfigFile, TestServerConfigReader)
{
    _vars["configfile"] = "anyname";
    _readConfigFile<ReadConfigFileTestTraits>(testDesc);
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string("10"));
}


BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()
