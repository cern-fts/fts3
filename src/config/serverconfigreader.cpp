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
#define FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGDIRECTORY_DEFAULT "/var/log/fts3"
#define FTS3_CONFIG_SERVERCONFIG_MESSAGINGDIRECTORY_DEFAULT "/var/lib/fts3"
#define FTS3_CONFIG_SERVERCONFIG_CONFIGFILE_DEFAULT "/etc/fts3/fts3config"
#define FTS3_CONFIG_SERVERCONFIG_MONFILE_DEFAULT "/etc/fts3/fts-msg-monitoring.conf"
#define FTS3_CONFIG_SERVERCONFIG_DBTYPE_DEFAULT "mysql"
#define FTS3_CONFIG_SERVERCONFIG_DBTHREADS_DEFAULT "4"
#define FTS3_CONFIG_SERVERCONFIG_MAXPROCESSES_DEFAULT "12500"
#define FTS3_CONFIG_SERVERCONFIG_MAX_SUCCESS_RATE_DEFAULT 100
#define FTS3_CONFIG_SERVERCONFIG_MED_SUCCESS_RATE_DEFAULT 98
#define FTS3_CONFIG_SERVERCONFIG_LOW_SUCCESS_RATE_DEFAULT 97
#define FTS3_CONFIG_SERVERCONFIG_BASE_SUCCESS_RATE_DEFAULT 96
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
        "PidDirectory",
        po::value<std::string>( &(_vars["PidDirectory"]) )->default_value("/var/lib/fts3"),
        "Where to put the PID files"
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
        "Database backend type. Allowed values: mysql"
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
	 	"AuthorizationProvider,w",
	    po::value<std::string>( &(_vars["AuthorizationProvider"]) )->default_value(""),
	    "Authorization provider ex IAM"
	)
	(
		"AuthorizationProviderTokenEndpoint,w",
		po::value<std::string>( &(_vars["AuthorizationProviderTokenEndpoint"]) )->default_value(""),
		"Authorization token endpoint ex IAM"
	)
	(
		"AuthorizationProviderJwkEndpoint,w",
		po::value<std::string>( &(_vars["AuthorizationProviderJwkEndpoint"]) )->default_value(""),
		"Jwk enpoint of Authorization provider"
	)
    (
        "ClientId,w",
        po::value<std::string>( &(_vars["ClientId"]) )->default_value(""),
        "Authorization provider CLient id"
    )
	(
		"ClientSecret,w",
		po::value<std::string>( &(_vars["ClientSecret"]) )->default_value(""),
		"Authorization provider Client Secret"
	)
	(
		"TokenRefreshTimeSinceLastTransferInSeconds,w",
		po::value<std::string>( &(_vars["TokenRefreshTimeSinceLastTransferInSeconds"]) )->default_value(""),
		"Time interval since last sumbit for a user to have his token refreshed (i.e. a for a month inactive user should not have his token refreshed)"
	)
	(
		"TokenRefreshDaemonIntervalInSeconds,w",
		po::value<std::string>( &(_vars["TokenRefreshDaemonIntervalInSeconds"]) )->default_value(""),
		"The interval that the token refresh daemon will run"
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
        "The list of info providers ( e.g.: glue1:glue2)"
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
        po::value<std::string>( &(_vars["TransferLogDirectory"]) )->default_value(
            FTS3_CONFIG_SERVERCONFIG_TRANSFERLOGDIRECTORY_DEFAULT),
        "Directory where the transfer logs are written"
    )
    (
        "MessagingDirectory",
        po::value<std::string>( &(_vars["MessagingDirectory"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_MESSAGINGDIRECTORY_DEFAULT),
        "Directory where the internal FTS3 messages are written"
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
        "CleanInterval",
        po::value<std::string>( &(_vars["CleanInterval"]) )->default_value("7"),
        "In days. Entries older than this will be purged"
    )
    (
        "BackupTables",
        po::value<std::string>( &(_vars["BackupTables"]) )->default_value("true"),
        "Enable or disable the t_file and t_job backup"
    )
    (
        "CheckStalledTransfers",
        po::value<std::string>( &(_vars["CheckStalledTransfers"]) )->default_value("true"),
        "Check for stalled transfers"
    )
    (
        "CheckStalledTimeout",
        po::value<std::string>( &(_vars["CheckStalledTimeout"]) )->default_value("900"),
        "Timeout for stalled transfers, in seconds"
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
        po::value<std::string>( &(_vars["LogLevel"]) )->default_value("INFO"),
        "Logging level"
    )
    (
        "WithoutSoap",
        po::value<std::string>( &(_vars["WithoutSoap"]) )->default_value("false"),
        "Disable SOAP interface"
    )
    (
        "MonitoringConfigFile",
        po::value<std::string>( &(_vars["MonitoringConfigFile"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_MONFILE_DEFAULT),
        "Monitoring configuration file"
    )
    (
        "StagingBulkSize",
        po::value<std::string>( &(_vars["StagingBulkSize"]) )->default_value("200"),
        "Staging bulk size"
    )
    (
        "StagingConcurrentRequests",
        po::value<std::string>( &(_vars["StagingConcurrentRequests"]) )->default_value("1000"),
        "Maximum number of staging concurrent requests"
    )
    (
        "StagingWaitingFactor",
        po::value<std::string>( &(_vars["StagingWaitingFactor"]) )->default_value("300"),
        "Seconds to wait before submitting a bulk request, so FTS can accumulate more files per bulk"
    )
    (
        "StagingPollRetries",
        po::value<std::string>( &(_vars["StagingPollRetries"]) )->default_value("3"),
        "Retry this number of times if a staging poll fails with ECOMM"
    )
    (
        "HeartBeatInterval",
        po::value<std::string>( &(_vars["HeartBeatInterval"]) )->default_value("60"),
        "Interval in seconds between beats"
    )
    (
        "HeartBeatGraceInterval",
        po::value<std::string>( &(_vars["HeartBeatGraceInterval"]) )->default_value("120"),
        "After this many seconds, a host is considered to be down"
    )
    (
        "OptimizerSteadyInterval",
        po::value<std::string>( &(_vars["OptimizerSteadyInterval"]) )->default_value("300"),
        "After this time without optimizer updates, force a run"
    )
    (
        "OptimizerInterval",
        po::value<std::string>( &(_vars["OptimizerInterval"]) )->default_value("60"),
        "Seconds between optimizer runs"
    )
    (
        "OptimizerMaxStreams",
        po::value<std::string>( &(_vars["OptimizerMaxStreams"]) )->default_value("16"),
        "Maximum number of streams per file"
    )
    (
        "MaxUrlCopyProcesses",
        po::value<std::string>( &(_vars["MaxUrlCopyProcesses"]) )->default_value("400"),
        "Maximum number of url copy processes to run"
    )
    (
        "PurgeMessagingDirectoryInterval",
        po::value<std::string>( &(_vars["PurgeMessagingDirectoryInterval"]) )->default_value("600"),
        "In seconds, how often to purge the messaging directory"
    )
    (
        "CheckSanityStateInterval",
        po::value<std::string>( &(_vars["CheckSanityStateInterval"]) )->default_value("3600"),
        "In seconds, how often to run sanity checks"
    )
    (
        "MultihopSanityStateInterval",
        po::value<std::string>( &(_vars["MultihopSanityStateInterval"]) )->default_value("600"),
        "In seconds, how often to run multihop sanity checker"
    )
    (
        "CancelCheckInterval",
        po::value<std::string>( &(_vars["CancelCheckInterval"]) )->default_value("10"),
        "In seconds, how often to check for canceled transfers"
    )
    (
        "QueueTimeoutCheckInterval",
        po::value<std::string>( &(_vars["QueueTimeoutCheckInterval"]) )->default_value("300"),
        "In seconds, how often to check for expired queued transfers"
    )
    (
        "ActiveTimeoutCheckInterval",
        po::value<std::string>( &(_vars["ActiveTimeoutCheckInterval"]) )->default_value("300"),
        "In seconds, how often to check for stalled transfers"
    )
    (
        "SchedulingInterval",
        po::value<std::string>( &(_vars["SchedulingInterval"]) )->default_value("2"),
        "In seconds, how often to schedule new transfers"
    )
    (
        "MessagingConsumeInterval",
        po::value<std::string>( &(_vars["MessagingConsumeInterval"]) )->default_value("1"),
        "In seconds, how often to check for messages"
    )
	(
	    "OptimizerMaxSuccessRate",
	     po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_MAX_SUCCESS_RATE_DEFAULT),
	     "Percentage of the max success rate considered by the optimizer"
	)
	(
	  "OptimizerMedSuccessRate",
	   po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_MED_SUCCESS_RATE_DEFAULT),
	   "Percentage of the med success rate considered by the optimizer"
	)
	(
	   "OptimizerLowSuccessRate",
	    po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_LOW_SUCCESS_RATE_DEFAULT),
	   "Percentage of the low success rate considered by the optimizer"
	)
	(
	   "OptimizerBaseSuccessRate",
	    po::value<int>()->default_value(FTS3_CONFIG_SERVERCONFIG_BASE_SUCCESS_RATE_DEFAULT),
	    "Percentage of the base success rate considered by the optimizer"
	)
    (
        "OptimizerEMAAlpha",
        po::value<double>()->default_value(0.1),
        "Optimizer: EMA Alpha factor to reduce the influence of fluctuations"
    )
    (
        "OptimizerIncreaseStep",
        po::value<int>()->default_value(1),
        "Increase step size when the optimizer considers the performance is good"
    )
    (
        "OptimizerAggressiveIncreaseStep",
        po::value<int>()->default_value(2),
        "Increase step size when the optimizer considers the performance is good, and set to aggressive or normal"
    )
    (
        "OptimizerDecreaseStep",
        po::value<int>()->default_value(1),
        "Decrease step size when the optimizer considers the performance is bad"
    )
    (
        "SigKillDelay",
        po::value<std::string>( &(_vars["SigKillDelay"]) )->default_value("500"),
        "In milliseconds, delay between graceful SIGTERM and SIGKILL"
    )
    (   "AutoSessionReuse",
    	po::value<std::string>( &(_vars["AutoSessionReuse"]) )->default_value("false"),
	"Enable or disable auto session reuse"
    )
    (   "AutoSessionReuseMaxSmallFileSize",
    	po::value<int>()->default_value(104857600),
	"Max small file size for session reuse in bytes"
    )
    (   "AutoSessionReuseMaxBigFileSize",
	 po::value<long long>()->default_value(1073741824),
	 "Max big file size for session reuse in bytes"
    )
    (   "AutoSessionReuseMaxFiles",
         po::value<int>()->default_value(1000),
         "Max number of files per session reuse"
    )
    (   "AutoSessionReuseMaxBigFiles",
	 po::value<int>()->default_value(2),
	 "Max number of big files  per session reuse"
    )
    (
        "UseFixedJobPriority",
        po::value<std::string>( &(_vars["UseFixedJobPriority"]) )->default_value("0"),
        "Configure the system to use a fixed Job Priority, by default it queries the system to honour the priorities specified by the users"
    )
    (
        "CancelUnusedMultihopFiles",
        po::value<std::string>( &(_vars["CancelUnusedMultihopFiles"]) )->default_value("false"),
        "Enable or disable behaviour to cancel all NOT_USED files in a failed multihop job"
    )
    (
        "RetrieveSEToken",
        po::value<std::string>( &(_vars["RetrieveSEToken"]) )->default_value("false"),
        "Enable or disable retrieval of SE-issued tokens in the transfer agent"
    )
    (
        "BackwardsCompatibleProxyNames",
        po::value<std::string>( &(_vars["BackwardsCompatibleProxyNames"]) )->default_value("true"),
        "Enable or disable backwards compatible naming when searching for proxy credentials stored on the local file system."
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

template <typename T>
void ServerConfigReader::storeAsString(const std::string& aName)
{
    bool isFound = _vm.count(aName);
    assert(isFound);

    if (isFound) {
        _vars[aName] = boost::lexical_cast<std::string>(_vm[aName].as<T>());
    }
}

/* ---------------------------------------------------------------------- */

void ServerConfigReader::storeValuesAsStrings ()
{
    storeAsString("Port");
    storeAsString("ThreadNum");
    storeAsString("OptimizerMaxSuccessRate");
    storeAsString("OptimizerMedSuccessRate");
    storeAsString("OptimizerLowSuccessRate");
    storeAsString("OptimizerBaseSuccessRate");
    storeAsString<double>("OptimizerEMAAlpha");
    storeAsString("OptimizerIncreaseStep");
    storeAsString("OptimizerAggressiveIncreaseStep");
    storeAsString("OptimizerDecreaseStep");
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
