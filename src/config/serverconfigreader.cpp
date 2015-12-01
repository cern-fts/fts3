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

#include "ServerConfigReader.h"
#include <iostream>
#include <fstream>
#include "common/Exceptions.h"


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
    (
        "WithoutSoap",
        po::value<std::string>( &(_vars["WithoutSoap"]) )->default_value("false"),
        "Disable SOAP interface"
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
    static std::shared_ptr<std::istream> getStream (const std::string& aName)
    {
        std::shared_ptr<std::istream> in(dynamic_cast<std::istream*> (new std::ifstream(aName.c_str())));

        if (!(*in))
            {
                std::stringstream msg;
                msg << "Error opening file " << aName;
                throw SystemError(msg.str());
            }

        return in;
    }

    /* ---------------------------------------------------------------------- */

    static void processVariables(ServerConfigReader& reader)
    {
        reader.storeValuesAsStrings();
        reader.storeRoles ();
        reader.validateRequired ("SiteName");
    }
};

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
