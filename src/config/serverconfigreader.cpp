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

/** \file serverconfigreader.h Implementation of FTS3 server config reader. */

#include "serverconfigreader.h"
#include <iostream>
#include <fstream>
#include "common/Exceptions.h"

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW
#include "unittest/testsuite.h"
#include <boost/algorithm/string/find.hpp>
#include <cstdio>
#endif // FTS3_COMPILE_WITH_UNITTESTS


using namespace fts3::common;
using namespace fts3::config;


// Default config values
#define FTS3_CONFIG_SERVERCONFIG_PORT_DEFAULT 8443
#define FTS3_CONFIG_SERVERCONFIG_IP_DEFAULT "localhost"
#define FTS3_CONFIG_SERVERCONFIG_THREADNUM_DEFAULT 10
#define FTS3_CONFIG_SERVERCONFIG_SERVERLOGDIRECTOTY_DEFAULT ""
#define FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGDIRECTOTY_DEFAULT "/var/log/fts3"
#define FTS3_CONFIG_SERVERCONFIG_CONFIGFILE_DEFAULT "/etc/fts3/fts3config"
#define FTS3_CONFIG_SERVERCONFIG_DBTYPE_DEFAULT "oracle"
#define FTS3_CONFIG_SERVERCONFIG_DBTHREADS_DEFAULT "4"
#define FTS3_CONFIG_SERVERCONFIG_MAXPROCESSES_DEFAULT "12500"

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineGenericOptions()
{
    po::options_description generic("Generic options");
    generic.add_options()
    ("help,h", "Display this help page")
    ("version,v", "Display server version")
    ("no-daemon,n", "Do not daemonize")
    ("rush,r", "Start and stop faster. Not for use in production!")

    (
        "configfile,f",
        po::value<std::string>( &(_vars["configfile"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_CONFIGFILE_DEFAULT),
        "FTS3 server config file"
    );

    return generic;
}

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineConfigOptions()
{
    po::options_description config("Configuration");

    config.add_options()
    (
        "Port,p",
        po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_PORT_DEFAULT),
        "File transfer listening port"
    )
    (
        "DbThreadsNum,D",
        po::value<std::string>( &(_vars["DbThreadsNum"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_DBTHREADS_DEFAULT),
        "Number of db connections in the db threads pool"
    )
    (
        "MaxNumberOfProcesses,M",
        po::value<std::string>( &(_vars["MaxNumberOfProcesses"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_MAXPROCESSES_DEFAULT),
        "Maximum processes resource limit"
    )
    (
        "IP,i",
        po::value<std::string>( &(_vars["IP"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_IP_DEFAULT),
        "IP address that the server is bound to"
    )

    (
        "DbConnectString,s",
        po::value<std::string>( &(_vars["DbConnectString"]) )->default_value(""),
        "Connect string for the used database account"
    )

    (
        "DbType,d",
        po::value<std::string>( &(_vars["DbType"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_DBTYPE_DEFAULT),
        "Database backend type. Allowed values: oracle"
    )

    (
        "DbUserName,u",
        po::value<std::string>( &(_vars["DbUserName"]) )->default_value(""),
        "Database account user name"
    )

    (
        "DbPassword,w",
        po::value<std::string>( &(_vars["DbPassword"]) )->default_value(""),
        "Database account password"
    )
    (
        "Infosys",
        po::value<std::string>( &(_vars["Infosys"]) )->default_value("lcg-bdii.cern.ch:2170"),
        "Set infosys"
    )
    (
        "BDIIKeepAlive",
        po::value<std::string>( &(_vars["BDIIKeepAlive"]) )->default_value("true"),
        "Sets the keep alive property of the BDII"
    )
    (
        "MyOSG,m",
        po::value<std::string>( &(_vars["MyOSG"]) )->default_value("false"),
        "Set the MyOSG URL (or flase meaning MyOSG wont be used)"
    )
    (
        "InfoProviders",
        po::value<std::string>( &(_vars["InfoProviders"]) )->default_value("glue1"),
        "The list of info providers ( e.g.: glue1:myosg:glue2)"
    )
    (
        "InfoPublisher,P",
        po::value<std::string>( &(_vars["InfoPublisher"]) )->default_value("false"),
        "Set this VM to be the info provider for Glue2"
    )
    (
        "Alias,a",
        po::value<std::string>( &(_vars["Alias"]) )->default_value(""),
        "Set the alias for FTS 3 endpoint"
    )
    (
        "Optimizer,o",
        po::value<std::string>( &(_vars["Optimizer"]) )->default_value("true"),
        "Control auto-tunning activation"
    )
    (
        "CleanRecordsHost,C",
        po::value<std::string>( &(_vars["CleanRecordsHost"]) )->default_value("true"),
        "Set to true when this host will be cleaning old records from the database"
    )
    (
        "HttpKeepAlive,k",
        po::value<std::string>( &(_vars["HttpKeepAlive"]) )->default_value("true"),
        "Control HTTP Keep alive in gsoap"
    )
    (
        "ServerLogDirectory",
        po::value<std::string>( &(_vars["ServerLogDirectory"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_SERVERLOGDIRECTOTY_DEFAULT),
        "Directory where the service logs are written"
    )
    (
        "TransferLogDirectory,l",
        po::value<std::string>( &(_vars["TransferLogDirectory"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGDIRECTOTY_DEFAULT),
        "Directory where the transfer logs are written"
    )
    (
        "AuthorizedVO,v",
        po::value<std::string>( &(_vars["AuthorizedVO"]) )->default_value(std::string()),
        "List of authorized VOs"
    )
    (
        "roles.*",
        po::value<std::string>(),
        "Authorization rights definition."
    )
    (
        "SiteName",
        po::value<std::string>( &(_vars["SiteName"]) ),
        "Site name running the FTS3 service"
    )
    (
        "MonitoringMessaging",
        po::value<std::string>( &(_vars["MonitoringMessaging"]) )->default_value("true"),
        "Enable or disable monitoring using messaging for monitoring"
    )
    (
        "Profiling",
        po::value<std::string>( &(_vars["Profiling"]) )->default_value("0"),
        "Enable or disable internal profiling"
    )
    (
        "InternalThreadPool",
        po::value<std::string>( &(_vars["InternalThreadPool"]) )->default_value("5"),
        "Set the number of threads in the internal threadpool"
    )
    (
        "CleanBulkSize",
        po::value<std::string>( &(_vars["CleanBulkSize"]) )->default_value("5000"),
        "Set the bulk size, in number of jobs, used for cleaning the old records"
    )
    (
        "CheckStalledTransfers",
        po::value<std::string>( &(_vars["CheckStalledTransfers"]) )->default_value("true"),
        "Check for stalled transfers"
    )
    (
        "MinRequiredFreeRAM",
        po::value<std::string>( &(_vars["MinRequiredFreeRAM"]) )->default_value("50"),
        "Minimum amount of free RAM in MB required for FTS3 to not go into auto-drain mode"
    )
    (
        "User",
        po::value<std::string>( &(_vars["User"]) )->default_value("fts3"),
        "Use this user to run the service"
    )
    (
        "Group",
        po::value<std::string>( &(_vars["Group"]) )->default_value("fts3"),
        "Use this group to run the service"
    )
    (
        "LogLevel",
        po::value<std::string>( &(_vars["LogLevel"]) )->default_value("DEBUG"),
        "Logging level"
    )
    ;

    return config;
}

/* ---------------------------------------------------------------------- */

po::options_description ServerConfigReader::_defineHiddenOptions()
{
    po::options_description hidden("Hidden options");

    hidden.add_options()
    (
        "ThreadNum,t",
        po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_THREADNUM_DEFAULT),
        "Number of worker threads."
    );

    return hidden;
}

/* ========================================================================== */

/** Read command line option - the real thing. */
struct ReadCommandLineOptions_SystemTraits
{
    /*static void exit(const int aVal)
    {
        ::exit(aVal);
    }*/

    /* ---------------------------------------------------------------------- */

    static std::ostream& stream()
    {
        return std::cout;
    }

    /* ---------------------------------------------------------------------- */

    static void processVariables
    (
        ServerConfigReader& aReader
    )
    {
        aReader.storeValuesAsStrings ();
        aReader.storeRoles ();
    }
};

/* ========================================================================== */

/** Read config file - the real thing. */
struct ReadConfigFile_SystemTraits
{
    /*static void exit(const int aVal)
    {
        ::exit(aVal);
    }*/

    static std::shared_ptr<std::istream> getStream (const std::string& aName)
    {
        std::shared_ptr<std::istream> in
        (
            dynamic_cast<std::istream*> (new std::ifstream(aName.c_str()))
        );

        if (!(*in))
            {
                std::stringstream msg;
                msg << "Error opening file " << aName;
                throw SystemError(msg.str());
            }

        return in;
    }

    /* ---------------------------------------------------------------------- */

    static void processVariables
    (
        ServerConfigReader& reader
    )
    {
        reader.storeValuesAsStrings();
        reader.storeRoles ();
        reader.validateRequired ("SiteName");
    }
};

/* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readConfigFile_SystemTraits,
    ReadConfigFile_SystemTraits
)
{
    // Test non-existing file opening
    BOOST_CHECK_EXCEPTION
    (
        getStream("/atyala/patyala/thisfile-doesnot_exis"),
        fts3::common::SystemError,
        fts3_unittest_always_true
    );

    // Test opening existing file
    std::shared_ptr<std::istream> in = getStream ("/etc/group");
    BOOST_CHECK (in.get());
    BOOST_CHECK (*in);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ========================================================================== */

ServerConfigReader::type_return ServerConfigReader::operator() (int argc, char** argv)
{

    po::options_description generic = _defineGenericOptions();
    po::options_description config = _defineConfigOptions();
    po::options_description hidden = _defineHiddenOptions();

    // Option group in the command line
    po::options_description cmdline_options;
    cmdline_options.add(generic).add(config).add(hidden);
    _readCommandLineOptions<ReadCommandLineOptions_SystemTraits> (argc, argv, cmdline_options);

    // Option group in config file
    po::options_description config_file_options;
    config_file_options.add(config).add(hidden);
    _readConfigFile<ReadConfigFile_SystemTraits> (config_file_options);

    // For legacy configurations, point ServerLogDirectory to TransferLogDirectory if not configured
    if (_vars["ServerLogDirectory"].empty()) {
        _vars["ServerLogDirectory"] = _vars["TransferLogDirectory"];
    }

    return _vars;
}

/* ------------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

/** This test checks if command line options really change default values */

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

BOOST_FIXTURE_TEST_CASE (ServerConfigReader_functionOperator_default, ServerConfigReader)
{
    static const int argc = 5;
    char *argv[argc];
    argv[0] = const_cast<char*> ("executable");
    argv[1] = const_cast<char*> ("--configfile=/dev/null");
    argv[2] = const_cast<char*> ("--Port=7823682");
    argv[3] = const_cast<char*> ("--SiteName");
    argv[4] = const_cast<char*> ("required");

    (*this)(argc, argv);
    BOOST_CHECK_EQUAL (_vars["Port"], std::string("7823682"));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

/* ========================================================================== */

/** Test class for DbType options. */
struct Test_DbType_ServerConfigReader : public ServerConfigReader
{
    /* -------------------------------------------------------------------- */
    Test_DbType_ServerConfigReader()
        : argc (argc_max)
    {
        argv[0] = const_cast<char*> ("executable");
        argv[1] = const_cast<char*> ("--configfile=/dev/null");
        argv[2] = const_cast<char*> ("--SiteName");
        argv[3] = const_cast<char*> ("required");
    }

    /* -------------------------------------------------------------------- */

    void doTest()
    {
        (*this)(argc, argv);
        BOOST_CHECK_EQUAL (_vars["DbType"], label());
    }

    /* -------------------------------------------------------------------- */

    static const int argc_max = 6;

    /* -------------------------------------------------------------------- */

    static const std::string& label ()
    {
        static const std::string str("atyala");
        return str;
    }

    /* -------------------------------------------------------------------- */

    char* argv[argc_max];

    /* -------------------------------------------------------------------- */

    int argc;
};

/* ------------------------------------------------------------------------- */

/** This test checks if short DbType argument works */

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

BOOST_FIXTURE_TEST_CASE (ServerConfigReader_DbType_short, Test_DbType_ServerConfigReader)
{
    argv[4] = const_cast<char*> ("-d");
    argv[5] = const_cast<char*> (label().c_str());
    argc = 6;
    doTest();
}

/* ------------------------------------------------------------------------- */

/** This test checks if long DbType argument works */
BOOST_FIXTURE_TEST_CASE (ServerConfigReader_DbType_long, Test_DbType_ServerConfigReader)
{
    std::string opt =  std::string("--DbType=") + label();
    argv[4] = const_cast<char*> (opt.c_str());
    argc = 5;
    doTest();
}

/* ------------------------------------------------------------------------- */

/** This test checks if default DbType argument works */
BOOST_FIXTURE_TEST_CASE (ServerConfigReader_DbType_default, Test_DbType_ServerConfigReader)
{
    argc = 4;
    (*this)(argc, argv);
    BOOST_CHECK_EQUAL (_vars["DbType"], FTS3_CONFIG_SERVERCONFIG_DBTYPE_DEFAULT);
}

/* ------------------------------------------------------------------------- */


/** Check if you can specify all the options in a config file */
BOOST_FIXTURE_TEST_CASE (ServerConfigReader_functionOperator_fromfile, ServerConfigReader)
{
    // Open a temporary file
    char filename[L_tmpnam];
    std::tmpnam(filename);
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
    file.close();
    // Read from the file
    static const int argc = 4;
    char *argv[argc];
    argv[0] = const_cast<char*> ("executable");
    std::string confpar = std::string("--configfile=") + filename;
    argv[1] = const_cast<char*> (confpar.c_str());
    argv[2] = const_cast<char*> ("--SiteName");
    argv[3] = const_cast<char*> ("required");

    (*this)(argc, argv);
    // Do the checks
    BOOST_CHECK_EQUAL (_vars["Port"], f_intval);
    BOOST_CHECK_EQUAL (_vars["ThreadNum"], f_intval);
    BOOST_CHECK_EQUAL (_vars["IP"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbConnectString"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbUserName"], f_str);
    BOOST_CHECK_EQUAL (_vars["DbPassword"], f_str);
    BOOST_CHECK_EQUAL (_vars["TransferLogDirectory"], f_str);

    std::remove(filename);
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTEST

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

/** Traits to test _readCommandLineOptions */
struct readCommandLineOptions_TestTraits
{
    static void exit(const int)
    {
        exitCalled = true;
    }

    /* ---------------------------------------------------------------------- */

    static void processVariables
    (
        ServerConfigReader&
    )
    {
        processVariablesCalled = true;
    }

    /* ---------------------------------------------------------------------- */

    static void reset()
    {
        processVariablesCalled = false;
        exitCalled = false;
        strstream.str("");
    }

    /* ---------------------------------------------------------------------- */

    static std::ostream& stream()
    {
        return strstream;
    }

    /* ---------------------------------------------------------------------- */

    static bool processVariablesCalled;
    static bool exitCalled;
    static std::stringstream strstream;
};

bool readCommandLineOptions_TestTraits::processVariablesCalled;
bool readCommandLineOptions_TestTraits::exitCalled;
std::stringstream readCommandLineOptions_TestTraits::strstream;

/* ---------------------------------------------------------------------- */

struct TestServerConfigReader : public ServerConfigReader
{
    TestServerConfigReader()
    {
        argv[0] = const_cast<char*> ("executable");
        testDesc.add_options()("help,h", "Description");
        testDesc.add_options()("version", "Description");
        testDesc.add_options()("no-daemon,n", "Description");
        testDesc.add_options()("other", po::value<std::string>(), "Description");
        testDesc.add_options()("intpar", po::value<int>(), "Description");
    }

    /* ---------------------------------------------------------------------- */

    void setupParameters
    (
        const std::string& aOption
    )
    {
        readCommandLineOptions_TestTraits::reset();
        argv[1] = const_cast<char*> (aOption.c_str());
    }

    /* ---------------------------------------------------------------------- */

    /** This test checks if:
     *    - --help option recognized
     *    - help message displayed
     *    - program exits
     */
    void do_helpTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        std::string f_helpMessage("-h [ --help ]         Description");
        std::string displayedText = readCommandLineOptions_TestTraits::strstream.str();
        bool contained = boost::find_first (displayedText, f_helpMessage);
        BOOST_CHECK (contained);
    }

    /* ---------------------------------------------------------------------- */

    /** This test checks if:
     *    - version option displays FTS3 version string
     *    - program exits
     */
    void do_versionTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        std::string f_versionMessage(FTS3_SERVER_VERSION);
        std::string displayedText = readCommandLineOptions_TestTraits::strstream.str();
        bool contained = boost::find_first (displayedText, f_versionMessage);
        BOOST_CHECK (contained);
    }

    /* ---------------------------------------------------------------------- */

    /** This test checks if:
     *    - Any other options than helo or version calls provessVariables
     *    - program does not exit
     */
    void do_othersTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK ( ! readCommandLineOptions_TestTraits::exitCalled);
        BOOST_CHECK ( readCommandLineOptions_TestTraits::processVariablesCalled);
    }

    /* ---------------------------------------------------------------------- */

    /** This test checks the effect of nodaemon flags. */
    void do_noDaemonSpecifiedTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string("true"));
    }

    /* ---------------------------------------------------------------------- */

    /** This test checks the effect of nodaemon flags. */
    void do_noDaemonNotSpecifiedTest()
    {
        _readCommandLineOptions<readCommandLineOptions_TestTraits>(argc, argv, testDesc);
        BOOST_CHECK_EQUAL (_vars["no-daemon"], std::string());
    }

protected:

    /* ---------------------------------------------------------------------- */

    static const int argc = 2;
    char *argv[argc];
    po::options_description testDesc;
};

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_help_long,
    TestServerConfigReader
)
{
    // Test executing long help option
    setupParameters ("--help" );
    do_helpTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_help_short,
    TestServerConfigReader
)
{
    // Test executing short help option
    setupParameters ("-h" );
    do_helpTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_version,
    TestServerConfigReader
)
{
    // Test executing "version"
    setupParameters("--version");
    do_versionTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_other,
    TestServerConfigReader
)
{
    // Test executing "other" parameter. Test fixture requires paremeter to
    // other!
    setupParameters("--other=value");
    do_othersTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_nodaemon_long,
    TestServerConfigReader
)
{
    setupParameters ("--no-daemon" );
    do_noDaemonSpecifiedTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_readCommandLineOptions_nodaemon_short,
    TestServerConfigReader
)
{
    setupParameters ("-n" );
    do_noDaemonSpecifiedTest();
}

/* ---------------------------------------------------------------------- */

BOOST_FIXTURE_TEST_CASE
(
    Config_ServerConfigReader_readCommandLineOptions_nodaemon_not_specified,
    TestServerConfigReader
)
{
    setupParameters ("--help" );
    do_noDaemonNotSpecifiedTest();
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ========================================================================== */

void ServerConfigReader::storeAsString
(
    const std::string& aName
)
{
    bool isFound = _vm.count(aName);
    assert(isFound);

    if (isFound)
        {
            _vars[aName] = boost::lexical_cast<std::string>(_vm[aName].as<int>());
        }
}

/* ---------------------------------------------------------------------- */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

BOOST_FIXTURE_TEST_CASE
(
    ServerConfigReader_storeAsString,
    TestServerConfigReader
)
{
    setupParameters("--intpar=10");
    po::store(po::parse_command_line(argc, argv, testDesc), _vm);
    po::notify(_vm);
    // Execute test - with existing parameter
    storeAsString ("intpar");
    // Do the checks
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string ("10"));
}



BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS

/* ---------------------------------------------------------------------- */

void ServerConfigReader::storeValuesAsStrings ()
{
    storeAsString("Port");
    storeAsString("ThreadNum");
}

void ServerConfigReader::storeRoles ()
{
    po::variables_map::iterator it;
    for (it = _vm.begin(); it != _vm.end(); it++)
        {
            if (it->first.find("roles.") == 0)
                {
                    _vars[it->first] = it->second.as<std::string>();
                }
        }
}

void ServerConfigReader::validateRequired (std::string key)
{

    if (!_vm.count("SiteName"))
        throw UserError("The required configuration option: '" + key + "' has not been found!");
}

/* ========================================================================== */

#ifdef FTS3_COMPILE_WITH_UNITTEST_NEW

/** Traits to test _readConfigFile */
struct ReadConfigFile_TestTraits
{
    static void exit(const int)
    {
        // EMPTY
    }

    static std::shared_ptr<std::istream> getStream (const std::string&)
    {
        std::stringstream* ss = new std::stringstream;
        assert(ss);
        ss->str("");
        *ss << "intpar=10" << std::endl;
        std::shared_ptr<std::istream> ret (dynamic_cast<std::istream*>(ss));
        return ret;
    }

    /* ---------------------------------------------------------------------- */

    static void processVariables
    (
        ServerConfigReader& reader
    )
    {
        reader.storeAsString("intpar");
    }
};

/* ---------------------------------------------------------------------- */

BOOST_AUTO_TEST_SUITE( config )
BOOST_AUTO_TEST_SUITE(ServerConfigReaderSuite)

BOOST_FIXTURE_TEST_CASE (ServerConfigReader_readConfigFile, TestServerConfigReader)
{
    _vars["configfile"] = "anyname";
    _readConfigFile<ReadConfigFile_TestTraits>(testDesc);
    BOOST_CHECK_EQUAL (_vars["intpar"], std::string("10"));
}

BOOST_AUTO_TEST_SUITE_END()
BOOST_AUTO_TEST_SUITE_END()

#endif // FTS3_COMPILE_WITH_UNITTESTS
