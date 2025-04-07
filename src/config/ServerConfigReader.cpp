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
        "Alias,a",
        po::value<std::string>( &(_vars["Alias"]) )->default_value(""),
        "Set the alias for FTS 3 endpoint"
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
        "UrlCopyProcessPingInterval",
        po::value<std::string>( &(_vars["UrlCopyProcessPingInterval"]) )->default_value("60"),
        "Set the UrlCopyProcess ping and transfer update interval"
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
        "Profiling",
        po::value<std::string>( &(_vars["Profiling"]) )->default_value("false"),
        "Enable or disable internal profiling logs"
    )
    (
        "LogTokenRequests",
        po::value<std::string>( &(_vars["LogTokenRequests"]) )->default_value("false"),
        "Log HTTP content of token requests"
    )
    (
        "MonitoringConfigFile",
        po::value<std::string>( &(_vars["MonitoringConfigFile"]) )->default_value(FTS3_CONFIG_SERVERCONFIG_MONFILE_DEFAULT),
        "Monitoring configuration file"
    )
    (
        "FetchArchivingLimit",
        po::value<std::string>( &(_vars["FetchArchivingLimit"]) )->default_value("50000"),
        "Maximum number of archive monitoring operations to fetch from the database"
    )
    (
        "ArchivePollBulkSize",
        po::value<std::string>( &(_vars["ArchivePollBulkSize"]) )->default_value("200"),
        "Archive polling bulk size"
    )
    (
        "ArchivePollRetries",
        po::value<std::string>( &(_vars["ArchivePollRetries"]) )->default_value("3"),
        "Retry this number of times if an archive poll fails with ECOMM"
    )
    (
        "ArchivePollInterval",
        po::value<std::string>( &(_vars["ArchivePollInterval"]) )->default_value("600"),
        "Time interval between consecutive archive poll tasks"
    )
    (
        "ArchivePollSchedulingInterval",
        po::value<std::string>( &(_vars["ArchivePollSchedulingInterval"]) )->default_value("60"),
        "In seconds, how often to run the scheduler for archive poll operations"
    )
    (
        "FetchStagingLimit",
        po::value<std::string>( &(_vars["FetchStagingLimit"]) )->default_value("50000"),
        "Maximum number of staging requests to fetch from the database"
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
        "StagingSchedulingInterval",
        po::value<std::string>( &(_vars["StagingSchedulingInterval"]) )->default_value("60"),
        "In seconds, how often to run the scheduler for staging operations"
    )
    (
        "StagingPollRetries",
        po::value<std::string>( &(_vars["StagingPollRetries"]) )->default_value("3"),
        "Retry this number of times if a staging poll fails with ECOMM"
    )
    (
        "StagingPollInterval",
        po::value<std::string>( &(_vars["StagingPollInterval"]) )->default_value("600"),
        "Time interval between consecutive staging poll tasks"
    )
    (
        "DefaultBringOnlineTimeout",
        po::value<std::string>( &(_vars["DefaultBringOnlineTimeout"]) )->default_value("604800"),
        "Default bring-online timeout for BringOnline operations"
    )
    (
        "DefaultCopyPinLifetime",
        po::value<std::string>( &(_vars["DefaultCopyPinLifetime"]) )->default_value("604800"),
        "Default copy-pin-lifetime for BringOnline operations"
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
        "OptimizerThreadPool",
        po::value<std::string>( &(_vars["OptimizerThreadPool"]) )->default_value("10"),
        "Set the number of threads for the Optimizer threadpool"
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
        "ForceStartTransfersCheckInterval",
        po::value<std::string>( &(_vars["ForceStartTransfersCheckInterval"]) )->default_value("30"),
        "In seconds, how often to check for transfers to force start"
    )
    (
        "TokenExchangeCheckInterval",
        po::value<std::string>( &(_vars["TokenExchangeCheckInterval"]) )->default_value("30"),
        "In seconds, how often to check for tokens eligible for token-exchange (empty refresh token)"
    )
    (
        "TokenExchangeBulkSize",
        po::value<std::string>( &(_vars["TokenExchangeBulkSize"]) )->default_value("500"),
        "Bulk size for how many tokens to retrieve from the database at one time for token-exchange"
    )
    (
        "TokenRefreshCheckInterval",
        po::value<std::string>( &(_vars["TokenRefreshCheckInterval"]) )->default_value("15"),
        "In seconds, how often to check for tokens marked for refreshing"
    )
    (
        "TokenRefreshPollerInterval",
        po::value<std::string>( &(_vars["TokenRefreshPollerInterval"]) )->default_value("15"),
        "In seconds, how often to process ZMQ token-refresh requests"
    )
    (
        "TokenRefreshBulkSize",
        po::value<std::string>( &(_vars["TokenRefreshBulkSize"]) )->default_value("500"),
        "Bulk size for how many tokens to retrieve from the database at one time for token-refresh"
    )
    (
        "TokenRefreshSafetyPeriod",
        po::value<std::string>( &(_vars["TokenRefreshSafetyPeriod"]) )->default_value("300"),
        "In seconds, period before access token expiry up to which still consider access token valid"
    )
    (
        "TokenRefreshMarginPeriod",
        po::value<std::string>( &(_vars["TokenRefreshMarginPeriod"]) )->default_value("300"),
        "In seconds, period before token expiry at which the transfer agent will request a token refresh"
    )
    (
        "MessagingConsumeGraceTime",
        po::value<std::string>( &(_vars["MessagingConsumeGraceTime"]) )->default_value("600"),
        "In seconds, time window since last MessageProcessingService run to be considered inactive"
    )
    (
        "CancelCheckGraceTime",
        po::value<std::string>( &(_vars["CancelCheckGraceTime"]) )->default_value("1800"),
        "In seconds, time window since last CancelerService run to be considered inactive"
    )
    (
        "SchedulingGraceTime",
        po::value<std::string>( &(_vars["SchedulingGraceTime"]) )->default_value("600"),
        "In seconds, time window since last SchedulerService run to be considered inactive"
    )
    (
        "SupervisorGraceTime",
        po::value<std::string>( &(_vars["SupervisorGraceTime"]) )->default_value("600"),
        "In seconds, time window for the SupervisorService since last run to be considered inactive"
    )
    (
        "TokenExchangeCheckGraceTime",
        po::value<std::string>( &(_vars["TokenExchangeCheckGraceTime"]) )->default_value("600"),
        "In seconds, time window since last TokenExchangeService run to be considered inactive"
    )
    (
        "TokenRefreshGraceTime",
        po::value<std::string>( &(_vars["TokenRefreshGraceTime"]) )->default_value("120"),
        "In seconds, time window since last TokenRefreshService run to be considered inactive"
    )
    (
        "OptimizerGraceTime",
        po::value<std::string>( &(_vars["OptimizerGraceTime"]) )->default_value("3600"),
        "In seconds, time window since last OptimizerService run to be considered inactive"
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
    (
        "AllowSessionReuse",
        po::value<std::string>( &(_vars["AllowSessionReuse"]) )->default_value("true"),
        "Enable or disable session reuse transfers (default true)"
    )
    (
        "AllowJobPriority",
        po::value<std::string>( &(_vars["AllowJobPriority"]) )->default_value("true"),
        "Enable or disable job priorities (default true)"
    )
    (
        "UseFixedJobPriority",
        po::value<std::string>( &(_vars["UseFixedJobPriority"]) )->default_value("0"),
        "Schedule only transfers of a certain job priority. When 0, honor the user-specified priorities (default value = 0)"
    )
    (
        "CancelUnusedMultihopFiles",
        po::value<std::string>( &(_vars["CancelUnusedMultihopFiles"]) )->default_value("false"),
        "Enable or disable behaviour to cancel all NOT_USED files in a failed multihop job"
    )
    (
        "ExperimentalPostgresSupport",
        po::value<std::string>( &(_vars["ExperimentalPostgresSupport"]) )->default_value("false"),
        "Enable or disable experimental features of using PostgreSQL"
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
        reader.validateRequired("SiteName");
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

template <>
void ServerConfigReader::storeAsString<int>(const std::string& aName)
{
    bool isFound = _vm.count(aName);
    assert(isFound);

    if (isFound) {
        _vars[aName] = boost::lexical_cast<std::string>(_vm[aName].as<int>());
    }
}

/* ---------------------------------------------------------------------- */

void ServerConfigReader::storeValuesAsStrings ()
{
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

void ServerConfigReader::validateRequired(const std::string& key)
{
    if (!_vm.count("SiteName"))
        throw UserError("The required configuration option: '" + key + "' has not been found!");
}
