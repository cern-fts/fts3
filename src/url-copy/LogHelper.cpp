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

#include <gfal_api.h>
#include <boost/filesystem/path.hpp>
#include <iomanip>
#include <sstream>
#include <boost/filesystem/operations.hpp>
#include "LogHelper.h"


static void gfal2LogCallback(const gchar *, GLogLevelFlags, const gchar *message, gpointer)
{
    if (message) {
        FTS3_COMMON_LOGGER_NEWLOG(DEBUG) << message << fts3::common::commit;
    }
}


void setupLogging(unsigned debugLevel)
{
    switch (debugLevel) {
        case 3:
            setenv("CGSI_TRACE", "1", 1);
            setenv("GLOBUS_FTP_CLIENT_DEBUG_LEVEL", "255", 1);
            setenv("GLOBUS_FTP_CONTROL_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_AUTHZ_DEBUG_LEVEL", "2", 1);
            setenv("GLOBUS_CALLOUT_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSI_CERT_UTILS_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSI_CRED_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_PROXY_DEBUG_LEVEL", "10", 1);
            setenv("GLOBUS_GSI_SYSCONFIG_DEBUG_LEVEL", "1", 1);
            setenv("GLOBUS_GSI_GSS_ASSIST_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_GSSAPI_DEBUG_LEVEL", "5", 1);
            setenv("GLOBUS_NEXUS_DEBUG_LEVEL", "1", 1);
            setenv("GLOBUS_GIS_OPENSSL_ERROR_DEBUG_LEVEL", "1", 1);
            setenv("XRD_LOGLEVEL", "Dump", 1);
            setenv("GFAL2_GRIDFTP_DEBUG", "1", 1);
        case 2:
            setenv("CGSI_TRACE", "1", 1);
            setenv("GLOBUS_FTP_CLIENT_DEBUG_LEVEL", "255", 1);
            setenv("GLOBUS_FTP_CONTROL_DEBUG_LEVEL", "10", 1);
            setenv("GFAL2_GRIDFTP_DEBUG", "1", 1);
        case 1:
            gfal2_log_set_level(G_LOG_LEVEL_DEBUG);
            break;
        default:
            gfal2_log_set_level(G_LOG_LEVEL_MESSAGE);
            break;
    }

    gfal2_log_set_handler((GLogFunc) gfal2LogCallback, NULL);

    if (debugLevel >= 3) {
        fts3::common::theLogger().setLogLevel(fts3::common::Logger::TRACE);
    }
    else if (debugLevel >= 2) {
        fts3::common::theLogger().setLogLevel(fts3::common::Logger::DEBUG);
    }
    else {
        fts3::common::theLogger().setLogLevel(fts3::common::Logger::INFO);
    }
}


std::string generateLogPath(const std::string &baseDir, const Transfer &transfer)
{
    boost::filesystem::path logDir(baseDir);
    boost::filesystem::path logName(transfer.getTransferId());
    boost::filesystem::path fullLog = logDir / logName;

    return fullLog.native();
}


static std::string dateDir()
{
    // add date
    time_t current;
    time(&current);
    struct tm * date = gmtime(&current);

    // Create template
    std::stringstream ss;
    ss << std::setfill('0');
    ss << std::setw(4) << (date->tm_year + 1900)
    << "-" << std::setw(2) << (date->tm_mon + 1)
    << "-" << std::setw(2) << (date->tm_mday);

    return ss.str();
}


std::string generateArchiveLogPath(const std::string &baseDir, const Transfer &transfer)
{
    std::stringstream pairName;
    pairName << transfer.source.host << "__" << transfer.destination.host;

    boost::filesystem::path logDir(baseDir);
    boost::filesystem::path archiveDir =
        baseDir / boost::filesystem::path(dateDir()) / boost::filesystem::path(pairName.str());

    boost::filesystem::create_directories(archiveDir);

    boost::filesystem::path logName(transfer.getTransferId());
    boost::filesystem::path fullLog = archiveDir / logName;

    return fullLog.native();
}
